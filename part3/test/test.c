#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_START_ELEVATOR 335
#define __NR_ISSUE_REQUEST 336
#define __NR_STOP_ELEVATOR 337

int start_elevator() {
	printf("Start elevator returned %d\n",syscall(__NR_START_ELEVATOR));
	return 0;
}

int issue_request(int start, int dest,int type) {
	printf("Issue request(Start %d,Dest %d, type %d) returns %d\n",start,dest,type,syscall(__NR_ISSUE_REQUEST, start, dest,type));
	return 0;
}

int stop_elevator() {
	printf("Stop elevator returned %d\n",syscall(__NR_STOP_ELEVATOR));
	return 0;
}




int rnd(int min, int max) {
	return rand() % (max+1-min)+min; //slight bias towards first k
}

int main(int argc, char **argv) {
	int i;
	if(argc!= 2)
	{
	printf("./text.x [start|stop|test1|test2|test3|invalid|stress]\n");	
	return 0;
	}
	if(strcmp(argv[1],"test1")==0) //Test allowed pick ups
	{
	 issue_request(1, 3,0);
	 issue_request(1, 3,0);
	 issue_request(1, 3,1);
	 issue_request(1, 3,1);
	 issue_request(1, 3,2);
	 issue_request(1, 3,2);
	}
	else if(strcmp(argv[1],"test2")==0) //test unallowed pick ups
	{
	issue_request(7, 5,2);
	 issue_request(7, 5,2);
	 issue_request(7, 5,1);
	 issue_request(7, 5,1);
	 issue_request(7, 5,0);
	 issue_request(7, 5,0);
	}
	else if(strcmp(argv[1],"test3")==0) //test scheduling
	{
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	 issue_request(1, 10,2);
	}
	else if(strcmp(argv[1],"stress")==0) //No upper limit, speed test as well
	{
		for(i = 0; i < 1000;++i)
		{
			issue_request(rnd(1,10),rnd(1,10),rnd(0,2));
		}
	}
	else if(strcmp(argv[1],"invalid")==0)
	{
	issue_request(5,5,2); 
	issue_request(0,11,3);
	issue_request(-123,4,2);
	issue_request(123,2,0);
	issue_request(3,11,1);
	issue_request(3,5,21);
	issue_request(3,-1,1);
	}
	else if(strcmp(argv[1],"start")==0)
		start_elevator();
	else if(strcmp(argv[1],"stop")==0)
		stop_elevator();
	else
		printf("./text.x [start|stop|test1|test2|test3|invalid|stress]\n");	
	return 0;
}
