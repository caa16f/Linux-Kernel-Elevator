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
#include <linux/linkage.h>
#include <linux/fault-inject.h>
#include <linux/gfp.h>
#include <linux/sched.h>

//Test comment 1234
MODULE_LICENSE("GPL");
MODULE_AUTHOR ("CARLOS ALFONSO, BRITTANY ZABAWA");
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
#define MAXLEN 8192

#ifndef __GFP_WAIT
#define __GFP_WAIT __GFP_RECLAIM
#endif

#define KFLAGS (__GFP_WAIT | __GFP_IO | __GFP_FS)



#define FLOOR_SLEEP 2
#define LOAD_SLEEP 1
#define GRAPES 0
#define SHEEP 1
#define WOLF 2
#define MAX_LOAD 10
#define ISSUE_ERROR 1

static struct file_operations fops;

static elevator_type elevator;
static building_type building;
static passenger_type * passenger;
//Continue adding here
static struct task_struct *thread_elevator; //stores the kthread running the elevaotr's active
static struct task_struct *thread_loader;

static struct mutex elevator_mutex;
static struct mutex building_mutex;
static struct mutex floor_mutex;
static char * currentMsg;
static char * printMovement;

static int msgLen;
static int read_p;

////////////////////////////////////////////

static int currentFloor = 1; //Keeping track of floor member variable
							 //is set as one just for testing basic up movement from elevator


////////////////////////////////////////






int runElevator(void * data)
{
	ssleep(FLOOR_SLEEP);

	passenger_type * person;


	while(!kthread_should_stop()){

		mutex_lock_interruptible(&elevator_mutex);
		elevator.targetFloor = -1;

		if(!list_empty(&elevator.passengers)){
			person = list_last_entry(&elevator.passengers, passenger_type, list);
			if(person->targetFloor > 0){
				elevator.targetFloor = person->targetFloor;
				printk("New floor destination %d\n", elevator.targetFloor);

			}
		}


	mutex_unlock(&elevator_mutex);

	mutex_lock_interruptible(&elevator_mutex);
	mutex_lock_interruptible(&building_mutex);

	if(list_empty(&elevator.passengers) && !list_empty(&building.waitingForService)){
		person = list_last_entry(&building.waitingForService, passenger_type, list);
		printk("Passenger type: %d\npassenger start: %d\npassenger dest: %d\n", person->type, person->startFloor, person->targetFloor);
		if(person->startFloor > 0){
			elevator.targetFloor = person->targetFloor;
			printk("New waiting passenger dest: %d\n", elevator.targetFloor);
			printk("Current floor %d\n", elevator.floor);
		}
	}


	mutex_unlock(&building_mutex);
	mutex_unlock(&elevator_mutex);


	mutex_lock(&elevator_mutex);
	if(elevator.targetFloor == -1 ){
		elevator.movement = IDLE;
	}else if(elevator.floor < elevator.targetFloor){
		elevator.movement = UP;
		elevator.floor++;
		printk("Moving up\n");
	}else if(elevator.floor > elevator.targetFloor){
		elevator.movement = DOWN;
		elevator.floor--;
		printk("Moving down\n");
	}else if(elevator.floor == elevator.targetFloor){
		elevator.movement = LOADING;
	}


	mutex_unlock(&elevator_mutex);
	}

	

	return 0;

}


int checkLoading(int type){
	struct list_head * ptr;
	passenger_type * person;
	bool containsSheep = false;
	bool containsWolf = false;
	bool containsGrapes = false;


	list_for_each(ptr, &elevator.passengers){
		person = list_entry(ptr, passenger_type, list);
		 if(person->type == SHEEP)
			containsSheep = true;

		 if(person->type == WOLF)
			containsWolf = true;

		 if(person->type == GRAPES)
	 		containsGrapes = true;


	}


	switch(type){

		case SHEEP:
			if(elevator.occupancy +1 <= MAX_LOAD && !containsWolf)
				return 1;
		break;

		case WOLF:
                        if(elevator.occupancy +1 <= MAX_LOAD)
                                return 1;
                break;

		case GRAPES:
                        if(elevator.occupancy +1 <= MAX_LOAD && !containsSheep)
                                return 1;
                break;


	}

	return 0;


}

void removeOccupancy(void){
	elevator.occupancy -= 1;

}


void addOccupancy(void){
	elevator.occupancy += 1;
}



void load_passenger(int floor){
	struct list_head * ptr;
	passenger_type * person;
	printk("Passenger Loading\n");

	list_for_each(ptr, &building.waitingForService){
		person = list_entry(ptr, passenger_type, list);
		if(person->startFloor == floor && checkLoading(person->type)){
			printk("Loading Passenger\n");
			list_move(&person->list, &elevator.passengers);
			addOccupancy();
		}

	}

}


void unload_passenger(int floor){
	struct list_head *ptr, *dummy;
	passenger_type * person;
	printk("Passenger Unloading\n");

	list_for_each_safe(ptr,dummy,&elevator.passengers){
		person = list_entry(ptr, passenger_type, list);
		if(person->targetFloor == floor){
			printk("Unloading Passenger\n");
			removeOccupancy();
			list_del(&person->list);
			kfree(person);
		}
	}

}


void lookAtFloor(int floor){
        mutex_lock_interruptible(&building_mutex);
        mutex_lock_interruptible(&elevator_mutex);

        if(!list_empty(&building.waitingForService) && elevator.shutdown != 1)
	        load_passenger(floor);
        mutex_unlock(&elevator_mutex);
        mutex_unlock(&building_mutex);

        mutex_lock(&elevator_mutex);
        if(!list_empty(&elevator.passengers))
                unload_passenger(floor);
        mutex_unlock(&elevator_mutex);


}



int runLoading(){

        ssleep(LOAD_SLEEP);
        lookAtFloor(elevator.floor);
	return 0;
}




/////////////////////////////////////////////////////////////////////////////////////////

extern int (*STUB_start_elevator)(void);
int start_elevator(void){
	printk("Starting Elevator\n");


	elevator.movement = IDLE; // current state
	elevator.floor = 1; //current floor
	elevator.targetFloor = 1; //destination floor
	elevator.occupancy =0; //number of people in elevator
	elevator.shutdown =0;

	return 0;

}

extern int (*STUB_issue_request)(int,int,int);
int issue_request(int sfloor, int tfloor, int passenger_type){

	printk("New request: %d, %d => %d\n", passenger_type, sfloor, tfloor);

	if(passenger_type < 0 || passenger_type > 2) return ISSUE_ERROR;
	if(sfloor < 1 || sfloor > 10) return ISSUE_ERROR;
	if(tfloor <1 || tfloor > 10) return ISSUE_ERROR;
	if(tfloor == sfloor) return ISSUE_ERROR;

	passenger = kmalloc(sizeof(passenger_type), KFLAGS);
	passenger->type = passenger_type;
	passenger->startFloor = sfloor;
	passenger->targetFloor = tfloor;

	printk("Passenger Type : %d\n passenger start: %d\n passenger dest: %d\n", passenger->type , passenger->startFloor, passenger->targetFloor);
	mutex_lock_interruptible(&building_mutex);
	list_add_tail(&passenger->list, &building.waitingForService);
	mutex_unlock(&building_mutex);

	return 0;
}

extern int (*STUB_stop_elevator)(void);
int stop_elevator(void){
	printk("Stopping Elevator\n");
	mutex_lock_interruptible(&elevator_mutex);
	elevator.shutdown = 1;
	mutex_unlock(&elevator_mutex);
	return 0;
}


void createSyscalls(void){
	STUB_start_elevator =& (start_elevator);
	STUB_issue_request =& (issue_request);
	STUB_stop_elevator =& (stop_elevator);

}

void removeSyscalls(void){
	STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;

}



void printElevatorStatus(char * msg){

	msgLen += snprintf(currentMsg , MAXLEN, "\n=======Elevator======\n");


	switch(elevator.movement){
		case IDLE:
			strcpy(printMovement, "IDLE");
			break;
		case UP:
                        strcpy(printMovement, "UP");
                        break;
		case DOWN:
                        strcpy(printMovement, "DOWN");
                        break;
		case LOADING:
                        strcpy(printMovement, "LOADING");
                        break;
		case OFFLINE:
                        strcpy(printMovement, "OFFLINE");
                        break;

	}

	msgLen += snprintf(currentMsg + msgLen, MAXLEN-msgLen, "Movement State: %s\n", printMovement);
	msgLen += snprintf(currentMsg + msgLen, MAXLEN-msgLen, "Current Floor: %d\n", elevator.floor+1);
	msgLen += snprintf(currentMsg + msgLen, MAXLEN-msgLen, "Passenger Count: %d\n", elevator.occupancy);


}


void printBuildingStatus(char * msg){
	int startFloor;
	int waiting[10];
	int totalWaiting = 0;

	msgLen += snprintf(currentMsg + msgLen, MAXLEN-msgLen, "Number of passengers serviced: %d\n", building.serviced);


	struct list_head * ptr;
	passenger_type * person;

	int j = 0;
	for(; j < 10; j++){
	waiting[j] = 0;
	}


	//Count the number of passengers waiting on each floor
	if(!list_empty(&building.waitingForService)){
		list_for_each(ptr, &building.waitingForService){
			person = list_entry(ptr,passenger_type,list);
			startFloor = person->startFloor-1;
			waiting[startFloor] += 1;
			totalWaiting++;
		}
	}

	msgLen += snprintf(currentMsg + msgLen, MAXLEN-msgLen, "Number of passengers waiting: %d\n", totalWaiting);


	int i;
	for(i = 0; i < 10; i++){
		if(elevator.floor == i){
			msgLen += snprintf(currentMsg + msgLen, MAXLEN-msgLen, "[*] Floor %d: %d\n", i+1, waiting[i]);
		}else{
			msgLen += snprintf(currentMsg + msgLen, MAXLEN-msgLen, "[] Floor %d: %d\n", i+1, waiting[i]);

		}

	}



}




int elevator_open(struct inode *sp_inode, struct file *sp_file){
	int printElevator = 0;
	int printBuilding = 0;


	msgLen = 0;
	read_p = 1;
	currentMsg = kmalloc(sizeof(char) * MAXLEN, KFLAGS);
	printMovement = kmalloc(sizeof(char) * 50, KFLAGS);

	printk("Proc elevator open\n");

	while(!printElevator){
	mutex_lock_interruptible(&elevator_mutex);
	printElevatorStatus(currentMsg);
	printElevator = 1;
	mutex_unlock(&elevator_mutex);
	}

	while(!printBuilding){
		mutex_lock_interruptible(&building_mutex);
		printBuildingStatus(currentMsg);
		printBuilding = 1;
		mutex_unlock(&building_mutex);
	}

	if(currentMsg == NULL){
		printk("Error in elevator open");
		return -ENOMEM;
	}

	return 0;
}

ssize_t elevator_read(struct file *sp_file, char __user * buf, size_t size, loff_t *offset){
	printk("Elevator Read Called");

	read_p = !read_p;
	if(read_p)
		return 0;

	copy_to_user(buf,currentMsg,strlen(currentMsg));
	return strlen(currentMsg);
}


int elevator_release(struct inode *sp_inode, struct file *sp_file){
	kfree(currentMsg);
	return 0;
}


static int elevator_init(void){
	printk("Elevator Initializing\n");

	createSyscalls();


	//fops calls
	fops.open = elevator_open;
	fops.read = elevator_read;
	fops.release = elevator_release;
	
	
	INIT_LIST_HEAD(&elevator.passengers);
	INIT_LIST_HEAD(&building.waitingForService);

	//initalize the elevator locks
	mutex_init(&elevator_mutex);
	mutex_init(&building_mutex);
	
	
	
	
        thread_elevator = kthread_run(runElevator, NULL, "elevator thread");
	if(IS_ERR(thread_elevator)){
		printk("Error encountered in kthread_run of elevator thread");
		return PTR_ERR(thread_elevator);
	}
	
	thread_loader = kthread_run(runLoading, NULL, "Loader Thread");
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
module_init(elevator_init);


static void elevator_exit(void)
{
	struct list_head *ptr, *dummy;
	passenger_type * person;
	int output = -1;



	if(!list_empty(&building.waitingForService)){
		list_for_each_safe(ptr,dummy,&building.waitingForService){
			person = list_entry(ptr, passenger_type, list);
			list_del(ptr);
			kfree(person);
		}
	}


        if(!list_empty(&elevator.passengers)){
                list_for_each_safe(ptr,dummy,&elevator.passengers){
                        person = list_entry(ptr, passenger_type, list);
                        list_del(ptr);
                        kfree(person);
                }
        }

	output = kthread_stop(thread_elevator);
	if(output != -EINTR)
		printk("Elevator thread stopped successfully\n");
	else
		printk("Error in stopping elevator thread   %d\n",output);


	output = kthread_stop(thread_loader);
        if(output != -EINTR) 
                printk("Loader thread stopped successfully\n");
        else
                printk("Error in stopping loader thread   %d\n", output);

	removeSyscalls();
	remove_proc_entry(ENTRY_NAME,NULL);


	printk(KERN_NOTICE "Removing /proc/%s.\n", ENTRY_NAME);
}

module_exit(elevator_exit);
