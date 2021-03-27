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


void moveUP(void)
{
	ssleep(2);
	mutex_lock(&elevator_mutex);
	currentFloor++;
	//printk(KERN_ALERT "The elevator is at floor %d\n", currentFloor);
	mutex_unlock(&elevator_mutex);
	
	
}




int runElevator(void * data)
{

	//Basic running through Elevator up
	//Will run from first floor to top floor
	int i;
	for( i = 1; i <= 10; i++)
	{
	mutex_unlock(&elevator_mutex);
	printk(KERN_ALERT "[]Floor %d\n" , i);
	moveUP();
	mutex_lock(&elevator_mutex);
	}
	mutex_unlock(&elevator_mutex);
	
	return 0;
	
	
	
	
	
	//TODO: basic running though Elevator down
	






	/*if(elevaor.movement == IDLE)
	{
		//TODO: Make condition when elevator is in IDLE, and look through every floor
		//to see if anyone is waiting

	}
	else
	{
		
		
	}*/
}






/////////////////////////////////////////////////////////////////////////////////////////

extern long (*STUB_start_elevator)(void);
long start_elevator(void){
	printk("Starting Elevator\n");
	
	mutex_lock(&building_mutex); //not really sure what this does exactly unused
	mutex_lock(&elevator_mutex);
	
	//if elevator is already active 
	if(elevator.movement != OFFLINE)
	{
		mutex_unlock(&elevator_mutex); //let it continue to run there threads
		mutex_unlock(&building_mutex);
		return 1;
	}
	
	
	//TO DO: // set up default of carry items and passagers
	// Meaning: 10 passangers and 10 carry items 
	
	
	

	elevator.movement = IDLE; // current state
	elevator.floor = 1; //current floor
	elevator.targetFloor = 1; //destination floor
	elevator.occupancy =0; //number of people in elevator
	elevator.shutdown =0;
	
	mutex_unlock(&elevator_mutex);
	printk(KERN_INFO "STARTING ELEVATOR THREAD\n" );
	
	//TO DO: running elevator function
	//This is basic testing for up motion of elevator
	thread_elevator = kthread_run(runElevator, NULL, "ELEVATOR SIMIULATION");
	printk(KERN_INFO "STARTING BUILDING THREAD\n" ); 
	mutex_unlock(&building_mutex);
	
	
	

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

	//module installed, initial state OFFLINE
	elevator.movement =OFFLINE;
	
	
	INIT_LIST_HEAD(&elevator.passengers);
	INIT_LIST_HEAD(&building.waitingForService);

	//initalize the elevator locks
	mutex_init(&elevator_mutex);
	mutex_init(&building_mutex);
	
	
	//initalize the floor lock
	mutex_init(&floor_mutex);
	
	STUB_start_elevator = & (start_elevator);
	//STUB_issue_request = & (issue_request);
	STUB_stop_elevator = & (stop_elevator);
	
	

	    //TODO implement elevator_run
        //thread_elevator = kthread_run(elevator_run, NULL, "elevator thread");
	/*if(IS_ERR(thread_elevator)){
		printk("Error encountered in kthread_run of elevator thread");
		return PTR_ERR(thread_elevator);
	}

	//TODO implement loader_run
	//thread_loader = kthread_run(loader_run, NULL, "Loader Thread");
	if(IS_ERR(thread_loader)){
		printk("Error encountered in kthread_run of loader thread");
		return PTR_ERR(thread_loader);
	}*/

	building.serviced = 0;

	/*if(!proc_create(ENTRY_NAME, PERMS, NULL, &fops)){
		printk("ERROR IN elevator_init\n");
		remove_proc_entry(ENTRY_NAME,NULL);
		return -ENOMEM;
	}*/
	return 0;
}
module_init(elevator_init);


static void elevator_exit(void)
{
	STUB_start_elevator = NULL;
	//STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;

	stop_elevator();
	
	//remove_proc_entry(ENTRY_NAME,NULL);
	
	mutex_destroy(&elevator_mutex);	// Destroy/Clean locks
	mutex_destroy(&building_mutex);
	
	
	printk(KERN_NOTICE "Removing /proc/%s.\n", ENTRY_NAME);
}

module_exit(elevator_exit);


