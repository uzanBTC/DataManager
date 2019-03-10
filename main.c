#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "errmacros.h"
#include <pthread.h>
#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"
#include <sqlite3.h>
#include <wait.h>
#include "sbuffer.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define LOOPS	5
#define CHILD_POS	"\t\t\t"
#define MAX 200


#define LOG_NAME "gateway.log"
#define FIFO_NAME "logFifo"
//const char * FIFO_NAME="LOGfifo";

sbuffer_t* buffer;
FILE *fp_sensor_map;
FILE *fp;
int result;
pthread_mutex_t mymutex;


void* thread_func_1();
void* thread_func_2();
void* thread_func_3();
void run_child ();
void fifo_send(char* send);
void sbuffer_close();



int main( int argc, char *argv[] ) 
{
	
		
	pid_t child_pid;
	child_pid=fork();
	int port_number;
	SYSCALL_ERROR(child_pid);
	if(argc<2){port_number=5678;}
	else
	{
		port_number= atoi(argv[1]);
	} 

	if(child_pid==0) //log event
	{
		run_child();
	}
	else  //the main process to run tm
	{
		
		int child_exit_status;
		pthread_mutex_init(&mymutex,NULL);		
		sbuffer_init(&buffer);
		fp_sensor_map=fopen("room_sensor.map","r");
		//FIFO
		int result=mkfifo(FIFO_NAME,0666);
		CHECK_MKFIFO(result); 
		fp = fopen(FIFO_NAME, "w");
		printf("syncing with reader ok\n");
		
		pthread_t thread1; 
		pthread_t thread2; 
		pthread_t thread3;
		if(pthread_create(&thread1,NULL,thread_func_1,&port_number)!=0){
			printf("Thread1 create failure\n");
			abort();		
		}
		if(pthread_create(&thread2,NULL,thread_func_2,NULL)!=0){
			printf("Thread2 create failure\n");
			abort();
		}
		if(pthread_create(&thread3,NULL,thread_func_3,NULL)!=0){
			printf("Thread3 create failure\n");
			abort();
		}

		pthread_join(thread1,NULL);
		pthread_join(thread2,NULL);
		pthread_join(thread3,NULL);
		sbuffer_close();
		fclose(fp);
		fclose(fp_sensor_map);
		pthread_mutex_destroy(&mymutex);
		waitpid(child_pid, &child_exit_status,0);
	}	
	return 0;	
}

void *thread_func_1(void *port)
{
	int port_number=*(int*) port;
	connmgr_listen(port_number,&buffer);
	connmgr_free();
	return NULL;
}

void *thread_func_2()
{	
	datamgr_parse_sensor_data(fp_sensor_map,&buffer);
	datamgr_free();
	return NULL;
}

void *thread_func_3()
{
	DBCONN * db_conn=init_connection(1);
	storagemgr_parse_sensor_data(db_conn,&buffer);
	disconnect(db_conn);
	return NULL;
}

void run_child ()//fifo read
{
	FILE *fp_read;
	FILE *log;
	int result;
	char *str_result;
	char recv_buf[MAX];
	int counter=1;
	time_t ts;
	// reference toledo
	result = mkfifo(FIFO_NAME, 0666);
	CHECK_MKFIFO(result); 
	fp_read = fopen(FIFO_NAME, "r");
	log=fopen(LOG_NAME,"w");
	printf("syncing with writer ok\n");
	do
	{
		str_result=fgets(recv_buf,MAX,fp_read);
		if(str_result != NULL)
		{
			ts=time(NULL);			
			fprintf(log,"%s",str_result);
			printf("Message received: No.%d - Timestamp: %ld - %s\n", counter,ts,recv_buf);  
			counter++;
		}
	}while(str_result!=NULL);
	fclose(fp_read);
	fclose(log);
	exit(EXIT_SUCCESS);
}
void fifo_send(char* send)//fifo write
{
  	//reference toledo	
	pthread_mutex_lock(&mymutex);
	fputs(send,fp);
	fflush(fp);
	pthread_mutex_unlock(&mymutex);	
}

void sbuffer_close()
{
	sbuffer_free(&buffer);
	printf("Buffer has been freed\n");
}


