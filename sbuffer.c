#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "sbuffer.h"
#include <pthread.h>
#include <time.h> 
#include <unistd.h>
#include <sys/time.h>

int flag;//second time`s read delete the node
struct sbuffer_data {
    sensor_data_t data;
};

typedef struct sbuffer_node {
  struct sbuffer_node * next;
  sbuffer_data_t element;
} sbuffer_node_t;

struct sbuffer {
  sbuffer_node_t * head;
  sbuffer_node_t * tail;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  pthread_barrier_t barrier;
};	

int sbuffer_init(sbuffer_t ** buffer)
{
  *buffer = malloc(sizeof(sbuffer_t));
  flag=1;
  if (*buffer == NULL) return SBUFFER_FAILURE;
  (*buffer)->head = NULL;
  (*buffer)->tail = NULL;
  pthread_mutex_init(&((*buffer)->mutex),NULL);
  pthread_cond_init(&((*buffer)->cond),NULL);
  pthread_barrier_init(&((*buffer)->barrier),NULL,2);
  return SBUFFER_SUCCESS; 
}


int sbuffer_free(sbuffer_t ** buffer)
{
  pthread_mutex_destroy(&((*buffer)->mutex));
  pthread_barrier_destroy(&((*buffer)->barrier));
  pthread_cond_destroy(&((*buffer)->cond));
  if ((buffer==NULL) || (*buffer==NULL)) 
  {
    return SBUFFER_FAILURE;
  } 

  while ( (*buffer)->head )
  {
    sbuffer_node_t * dummy = (*buffer)->head;
    (*buffer)->head = (*buffer)->head->next;
    free(dummy);
  }
  free(*buffer);
  *buffer = NULL;
  return SBUFFER_SUCCESS;		
}


int sbuffer_remove(sbuffer_t * buffer,sensor_data_t * data)
{
  //if no data
  if (buffer == NULL) return SBUFFER_FAILURE;
 //https://blog.csdn.net/whz_zb/article/details/6937673
 //https://stackoverflow.com/questions/1486833/pthread-cond-timedwait

  if (buffer->head == NULL) 
  {
	 	
	struct timeval now;
        struct timespec outtime;
	pthread_mutex_lock(&((buffer)->mutex));
	//https://baike.baidu.com/item/gettimeofday/3369586
	//https://blog.csdn.net/wang_xijue/article/details/31758843 (reference)
	// if sbuffer time out, end the thread
	gettimeofday(&now, NULL);
	outtime.tv_sec=now.tv_sec+5;
	outtime.tv_nsec = now.tv_usec * 1000;
	int n=pthread_cond_timedwait(&((buffer)->cond),&((buffer)->mutex),&outtime);
	if(n==ETIMEDOUT){
		printf("Buffer Time out\n");
		pthread_mutex_unlock(&((buffer)->mutex));
		return SBUFFER_TIME_OUT;
	}
	else
	{
		pthread_mutex_unlock(&((buffer)->mutex));
		return SBUFFER_NO_DATA;
	}
  }
   
  //wait for two threads, in case one thread read the data twice
  pthread_barrier_wait(&((buffer)->barrier));
  pthread_mutex_lock(&((buffer)->mutex));
  
  *data = buffer->head->element.data;
  if(flag==0){
  	sbuffer_node_t *dummy = buffer->head;
  	if (buffer->head == buffer->tail) // buffer has only one node
  	{
  	  buffer->head = buffer->tail = NULL; 
  	}
  	else  // buffer has many nodes empty
  	{
  	  buffer->head = buffer->head->next;
  	}
  	flag=1;
  	free(dummy);
   }
 else{
	flag=0;
 }
  pthread_mutex_unlock(&((buffer)->mutex)); 
  sleep(1);
  return SBUFFER_SUCCESS;
}


int sbuffer_insert(sbuffer_t * buffer, sensor_data_t * data)
{
  pthread_mutex_lock(&((buffer)->mutex));
  sbuffer_node_t * dummy;
  if (buffer == NULL) return SBUFFER_FAILURE;
  dummy = malloc(sizeof(sbuffer_node_t));
  if (dummy == NULL) return SBUFFER_FAILURE;
  dummy->element.data = *data;
  dummy->next = NULL;
  if (buffer->tail == NULL) 
  {
    buffer->head = buffer->tail = dummy;
  } 
  else 
  {
    buffer->tail->next = dummy;
    buffer->tail = buffer->tail->next; 
  }
 pthread_mutex_unlock(&((buffer)->mutex));
 //broadcast to wake up the cond_wait
 pthread_cond_broadcast(&((buffer)->cond));
  return SBUFFER_SUCCESS;
}



