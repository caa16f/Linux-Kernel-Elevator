#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/string.h>

MODULE_LICENSE("Dual BSD/GPL");

#define BUF_LEN 100

static struct proc_dir_entry * proc_entry;

static char msg[BUF_LEN];
static char msg2[BUF_LEN];
static char fullMsg[BUF_LEN*2 + 1];

static char elaspedMsg[BUF_LEN];
static char elaspedMsg2[BUF_LEN];
static char fullElasped[BUF_LEN*2 +1];

static char currentMsg[10] = "Current: ";
static char elasped[9] = "Elasped: ";

static long long elaspedSeconds;
static long long elaspedNSeconds;

static bool firstRead = true;

static int procfs_buf_len;
struct timespec time_since_epoch;
struct timespec time_of_last_access;

struct timespec getTimeDifference(struct timespec time1, struct timespec time2);

static ssize_t procfile_read(struct file* file, char * ubuf, size_t count , loff_t *ppos)
{


	memset(fullMsg,0,sizeof(fullMsg));
	time_since_epoch = current_kernel_time();
	if(firstRead){
		time_of_last_access = time_since_epoch;
	}else{
		memset(fullElasped,0,sizeof(fullElasped));
		elaspedSeconds = time_of_last_access.tv_sec;
		elaspedNSeconds = time_of_last_access.tv_nsec;

		sprintf(elaspedMsg, "%ld", getTimeDifference(time_since_epoch, time_of_last_access).tv_sec);
		sprintf(elaspedMsg2, "%ld", getTimeDifference(time_since_epoch, time_of_last_access).tv_nsec);
		strcat(fullElasped, elaspedMsg);
		strcat(fullElasped, ".");
		strcat(fullElasped, elaspedMsg2);
		strcat(fullElasped, "\n");

		time_of_last_access = time_since_epoch;
	}

	sprintf(msg, "%ld", time_since_epoch.tv_sec);
	sprintf(msg2, "%ld" , time_since_epoch.tv_nsec);
	strcat(fullMsg, currentMsg);
	strcat(fullMsg, msg);
	strcat(fullMsg, ".");
	strcat(fullMsg, msg2);
	strcat(fullMsg, "\n");

	if(!firstRead){
		strcat(fullMsg, elasped);
		strcat(fullMsg, fullElasped);
	}


	printk(KERN_INFO "Starting Proc Read\n");
	procfs_buf_len = strlen(fullMsg) + strlen(fullElasped) + strlen(currentMsg)+ strlen(elasped) +2;

	if(*ppos > 0 || count < procfs_buf_len)	
		return 0;
	if(copy_to_user(ubuf, fullMsg, procfs_buf_len))
		return -EFAULT;

	*ppos = procfs_buf_len;

	printk(KERN_INFO "gave to user %s\n", msg);
	firstRead = false;
	return procfs_buf_len;


}

static ssize_t procfile_write(struct file* file, const char * ubuf, size_t count , loff_t * ppos){

	printk(KERN_INFO "Starting Proc Write\n");
	
	
		
	strcat(fullMsg, msg);
	strcat(fullMsg, msg2);

	if(count > BUF_LEN)
		procfs_buf_len = BUF_LEN;
	else
		procfs_buf_len = count;

	copy_from_user(msg, ubuf, procfs_buf_len);

	printk(KERN_INFO "got from user %s\n", fullMsg);

	return procfs_buf_len;

}

static struct file_operations procfile_fops = {
	.owner = THIS_MODULE,
	.read = procfile_read,
	.write = procfile_write,
};




static int __init my_timer_init(void){

	time_since_epoch = current_kernel_time();
	
	proc_entry =  proc_create("timer", 0666, NULL, &procfile_fops);

	if(proc_entry == NULL)
		return -ENOMEM;


	return 0;

}

static void __exit my_timer_exit(void){
	proc_remove(proc_entry);
	return;
}

struct timespec getTimeDifference(struct timespec time1, struct timespec time2){
	struct timespec result;
	if(time1.tv_nsec < time2.tv_nsec){
		int nsec = (time2.tv_nsec - time1.tv_nsec) /1000000000 +1;
		time2.tv_nsec -= 1000000000 * nsec;
		time2.tv_sec += nsec;
	}
	if(time1.tv_nsec - time2.tv_nsec > 1000000000){
		int nsec = (time1.tv_nsec - time2.tv_nsec) / 1000000000;
		time2.tv_nsec += 1000000000 *nsec;
		time2.tv_sec -= nsec;
	}
	result.tv_sec = time1.tv_sec - time2.tv_sec;
	result.tv_nsec = time1.tv_nsec - time2.tv_nsec;

	return result;

}


module_init(my_timer_init);
module_exit(my_timer_exit);
