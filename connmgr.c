#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "config.h"
#include <poll.h>
#include "lib/dplist.h"
#include "lib/tcpsock.h"
#include "connmgr.h"
#include <time.h> 
#include <unistd.h>
#include "sbuffer.h"

#ifndef TIMEOUT
  #define TIMEOUT 5
#endif

#ifndef MAX_CONN 
  #define MAX_CONN 1000
#endif

dplist_t *sensor_conn_list;
typedef struct sensor_conn
{
	int fd;
	tcpsock_t* con;
	time_t timestamp;
	int new_data;   //if the node is a new one
	int id;
}sensor_conn_t;


int element_compare_con(void * x, void * y)//compare sensor_id
{
  int fdx=((sensor_conn_t*) x)->fd;
  int fdy=((sensor_conn_t*) y)->fd;
  if(fdx==fdy){return 0;}
  else return -1;
}

void* element_copy_con(void * element)
{
    sensor_conn_t *e = (sensor_conn_t *)element;
    sensor_conn_t *copy = malloc(sizeof(sensor_conn_t));
    copy->fd=e->fd;
    (copy->con)=(e->con);
    copy->timestamp=e->timestamp;
    copy->id=e->id;
    copy->new_data=e->new_data;

    return (void*)copy;
}

void element_free_con(void ** element)
{
    free((*element));
    *element = NULL;
}

void connmgr_listen(int port_number,sbuffer_t ** buffer)
{
	tcpsock_t *server, *client;
	sensor_data_t data;	
	FILE *fp;
    	fp = fopen( "sensor_data_recv", "w");	
	printf("Test server is started\n");
	int tcp_result=tcp_passive_open(&server,port_number);
	if(tcp_result==TCP_NO_ERROR){printf("Server open successfully.\n");}
	else {printf("Server failure");exit(EXIT_FAILURE);}
	sensor_conn_list=dpl_create(element_copy_con,element_free_con,element_compare_con);
	struct pollfd fds[MAX_CONN];
	//asign server`s fd
	int server_sd;
	tcp_get_sd(server,&server_sd);
	fds[0].fd=server_sd; // standard input
	fds[0].events=POLLIN;
	int server_time=time(NULL);
	int fds_length=1;

	//initialize the fds
	for(int i=1;i<MAX_CONN;i++)
	{
		fds[i].fd=-1;  //when fd is negative, poll will skip it.
	}
	
	while(1)
	{		
				
		poll(fds,fds_length,0);
		//a new connection coming
		if(fds[0].revents & POLLIN) 
		{						
			if (tcp_wait_for_connection(server,&client)!=TCP_NO_ERROR) {printf("client connection failure;\n");exit(EXIT_FAILURE);}
			else {printf("client connection success;\n");}	
			
			int sd;
			tcp_get_sd(client,&sd);	
			fds_length++;

			if(fds_length>MAX_CONN){fds_length--;continue;}

			fds[fds_length-1].fd=sd;
			fds[fds_length-1].events=POLLIN;
			sensor_conn_t *sensor=malloc(sizeof(sensor_conn_t));
			sensor->fd=sd;
			sensor->timestamp=time(NULL);
			sensor->con=client;  
			sensor->new_data=1;
			sensor->id=-1;
			dpl_insert_at_index(sensor_conn_list, sensor, 0, true);
			free(sensor);  
			
		}
		
		//check every node if new data receive it
		for(int i=0;i<dpl_size(sensor_conn_list);i++)
		{
			sensor_conn_t *sensor=dpl_get_element_at_index(sensor_conn_list,i);			
			int pos=-1;			
			for(int j=1;j<fds_length;j++)
			{
				if(fds[j].fd==sensor->fd){pos=j;break;}
			}
			if(pos==-1){printf("searching error\n");continue;}
			if((fds[pos].revents & POLLIN)==POLLIN)
			{
				  int bytes, result;
				  
				  // read sensor 
				  bytes = sizeof(data.id);
				  tcp_receive(sensor->con,(void *)&data.id,&bytes);
				  // read temperature
				  bytes = sizeof(data.value);
				  tcp_receive(sensor->con,(void *)&data.value,&bytes);
				  // read timestamp
				  bytes = sizeof(data.ts);
				  result = tcp_receive(sensor->con, (void *)&data.ts,&bytes);

				//write to binary file
				if((result==TCP_NO_ERROR) && bytes)
				{
					printf("---------------------------------\n");	
					sensor->id=data.id;
					//write to FIFO				
					if((sensor->new_data)==1)
					{
					char* send=0;
					asprintf(&send,"A seneor node with ID=%d has opend a new connection success\n",data.id );
					
					fifo_send(send);
					free(send);
					sensor->new_data=0;
					}
					printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data.id, data.value, (long int)data.ts);

					//update sensor
					sensor->timestamp=data.ts;
					sbuffer_insert(*buffer,&data);				
				
				}
				else if(result==TCP_CONNECTION_CLOSED)
				{
					sleep(2);					
					int temp_id=sensor->id;					
					tcp_close(&(sensor->con));
					dpl_remove_at_index(sensor_conn_list,i,true);
					//write to FIFO for closed
					char* send=0;
					asprintf(&send,"A seneor node with ID=%d has closed success\n",temp_id);
					fifo_send(send);
					free(send);
					
					//rearrange the array to keep there is no empty space between nodes
					if(pos!=fds_length-1){  
					for(int i=pos;i<fds_length-1;i++)
					{
						fds[pos].fd=fds[pos+1].fd;
						fds[pos].events=fds[pos+1].events;
						fds[pos].revents=fds[pos+1].revents;
					}
					fds[fds_length-1].fd=-1;
					}
					else{
						fds[fds_length-1].fd=-1;
						fds_length--;
					}
				}
				else
				{
					printf("Connection error\n");
				}
			}
			//check client time out
			if((time(NULL)-sensor->timestamp)>TIMEOUT){
				printf("Connection timeout and close\n");				
				tcp_close(&(sensor->con));
				dpl_remove_at_index(sensor_conn_list,i,true);
				if(pos!=fds_length-1){  
					for(int i=pos;i<fds_length-1;i++)
						{
							fds[pos].fd=fds[pos+1].fd;
							fds[pos].events=fds[pos+1].events;
							fds[pos].revents=fds[pos+1].revents;
						}
					fds[fds_length-1].fd=-1;
				}
				else{
					fds[fds_length-1].fd=-1;
					fds_length--;
				}
					
			}
			//update the server time
			server_time=time(NULL);
		}
		//check server timeout
		if(dpl_size(sensor_conn_list)==0){
			if((time(NULL)-server_time)>TIMEOUT)
			{			
				tcp_close(&server);
				fclose(fp);
				fp=NULL;
				printf("Server timeout and close\n");
				break;
			}
		}

	}

}

void connmgr_free()
{
	dpl_free(&sensor_conn_list,true);
	sensor_conn_list=NULL;
}
