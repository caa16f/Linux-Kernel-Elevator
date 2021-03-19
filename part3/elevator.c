#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/fault-inject.h>
#include <linux/gfp.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR ("CARLOS ALFONSO");
MODULE_DESCRIPTION("Elevator Scheduler");

typedef enum {IDLE,UP,DOWN,LOADING,OFFLINE} movement_type;

typedef struct{
	int floor;
	int targetFloor;
	int occupancy;
	int load;
	int shutdown;
	movement_type movement;
	struct list_head passengers;
}elevator_type;

typedef struct{
	struct list_head waitingForService;
	int serviced;
}building_type;

typedef struct{
	struct list_head list;
	int type;
	int startFloor;
	int targetFloor;
}passenger_type;

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL
#define MAXLEN 2048

//TODO FIX THIS
//#define KFLAGS (__GFP_WAIT | __GFP_IO | __GFP_FS)

#define FLOOR_SLEEP 2
#define LOAD_SLEEP 1
#define SHEEP 0
#define GRAPES 1
#define WOLF 2
#define MAX_LOAD 10
#define ISSUE_ERROR 1

static struct file_operations fops;

static elevator_type elevator;
static building_type building;
static passenger_type * passenger;
//Continue adding here
static struct task_struct *thread_elevator;
static struct task_struct *thread_loader;

static struct mutex elevator_mutex;
static struct mutex building_mutex;

static char * currentMsg;
static char * printMovement;

static int msgLen;
static int read_p;

extern long (*STUB_start_elevator)(void);
long start_elevator(void){
	printk("Starting Elevator\n");

	elevator.movement = OFFLINE;
	elevator.floor = 1;
	elevator.targetFloor = 1;
	elevator.load = 0;
	elevator.occupancy =0;
	elevator.shutdown =0;

	return 0;

}

extern long (*STUB_issue_request)(int,int,int);
long issue_request(int passenger_type, int sfloor, int tfloor){

	printk("New request: %d, %d => %d\n", passenger_type, sfloor, tfloor);

	if(passenger_type < 0 || passenger_type > 2) return ISSUE_ERROR;
	if(sfloor < 1 || sfloor > 10) return ISSUE_ERROR;
	if(tfloor <1 || tfloor > 10) return ISSUE_ERROR;

	//TODO FIX KFLAGS NEED TO BE IN KERNEL NOT usr/src
	//passenger = kmalloc(sizeof(passenger_type), );
	passenger->type = passenger_type;
	passenger->startFloor = sfloor;
	passenger->targetFloor = tfloor;

	printk("Passenger Type : %d\n passenger start: %d\n passenger dest: %d\n", passenger->type , passenger->startFloor, passenger->targetFloor);
	mutex_lock_interruptible(&building_mutex);
	list_add_tail(&passenger->list, &building.waitingForService);
	mutex_unlock(&building_mutex);

	return 0;
}

extern long (*STUB_stop_elevator)(void);
long stop_elevator(void){
	printk("Stopping Elevator\n");
	mutex_lock_interruptible(&elevator_mutex);
	elevator.shutdown = 1;
	mutex_unlock(&elevator_mutex);
	return 0;
}

static int elevator_init(void){
	printk("Elevator Initializing\n");
	//TODO create syscalls with function here

	//TODO add fops calls

	INIT_LIST_HEAD(&elevator.passengers);
	INIT_LIST_HEAD(&building.waitingForService);

	mutex_init(&elevator_mutex);
	mutex_init(&building_mutex);

	//TODO implement elevator_run
        //thread_elevator = kthread_run(elevator_run, NULL, "elevator thread");
	if(IS_ERR(thread_elevator)){
		printk("Error encountered in kthread_run of elevator thread");
		return PTR_ERR(thread_elevator);
	}

	//TODO implement loader_run
	//thread_loader = kthread_run(loader_run, NULL, "Loader Thread");
	if(IS_ERR(thread_loader)){
		printk("Error encountered in kthread_run of loader thread");
		return PTR_ERR(thread_loader);
	}

	building.serviced = 0;

	if(!proc_create(ENTRY_NAME, PERMS, NULL, &fops)){
		printk("ERROR IN elevator_init\n");
		remove_proc_entry(ENTRY_NAME,NULL);
		return -ENOMEM;
	}
	return 0;
}
