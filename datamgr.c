#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h> 
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "datamgr.h"
#include "lib/dplist.h"


typedef struct {
  sensor_id_t sensor_id;
  sensor_id_t room_id;
  sensor_value_t avg_tmp;
  sensor_ts_t ts;
  sensor_value_t data_record[RUN_AVG_LENGTH];
  int counter;
} sensor_data_record_t;  //for recording the values

typedef struct {
  sensor_id_t sensor_id;
} sensor_working_node;  //for searching


int element_compare(void * x, void * y)//compare sensor_id
{
  sensor_id_t idx=((sensor_data_record_t*)x)->sensor_id;
  sensor_id_t idy=((sensor_data_record_t*)y)->sensor_id;
  if(idx==idy){return 0;}
  else if(idx>idy){return -1;}
  else if(idx<idy){return 1;}
  return -1;
}

void* element_copy(void * element)
{
    sensor_data_record_t *e = (sensor_data_record_t *)element;
    sensor_data_record_t *copy = malloc(sizeof(sensor_data_record_t));
    copy->sensor_id=e->sensor_id;
    copy->room_id=e->room_id;
    copy->ts=e->ts;
    copy->avg_tmp=e->avg_tmp;
    copy->ts=e->ts;
    copy->counter=e->counter;
    for(int i=0;i<RUN_AVG_LENGTH;i++)
    {
	copy->data_record[i]=e->data_record[i];
    }  
    return (void *)copy;
}

void element_free(void ** element)
{
    free((*element));
    *element = NULL;
}


dplist_t *sensor_list = NULL;

void datamgr_parse_sensor_files(FILE * fp_sensor_map,FILE * fp_sensor_data)
{
	ERROR_HANDLER(fp_sensor_map == NULL, "error");
    	ERROR_HANDLER(fp_sensor_data == NULL, "error");	
	//some variables to parse the file
	uint16_t sensor_id;
	sensor_value_t tmp;
	sensor_ts_t time;
	uint16_t room_id;
		
	//create sensor node list
	sensor_list=dpl_create(element_copy,element_free,element_compare);

	//read data from sensor map to get room id and sensor id and store them in node list
	while(fscanf(fp_sensor_map,"%16hu %16hu \n",&room_id,&sensor_id) !=EOF)
	{
		//create the element of the dplist		
		sensor_data_record_t* sensor_node=malloc(sizeof(sensor_data_record_t));
		sensor_node->sensor_id=sensor_id;
		sensor_node->room_id=room_id;
                dpl_insert_at_index(sensor_list,sensor_node,-1,1);//add the node to the head
		free(sensor_node);
	}

	//read data from sensor data file, a binary file => get the exact temperature value and calculate the average
	while(fread(&sensor_id,sizeof(uint16_t),1,fp_sensor_data))
	{
		fread(&tmp,sizeof(double),1,fp_sensor_data);
		fread(&time,sizeof(time_t),1,fp_sensor_data);
		//check if the sensor_id is in the list
		sensor_working_node* sensor_w_node=malloc(sizeof(sensor_working_node));
		sensor_w_node->sensor_id = (sensor_id);

		int index=dpl_get_index_of_element(sensor_list, sensor_w_node);
		if(index!=-1){
			sensor_data_record_t* sensor_node=(sensor_data_record_t*) dpl_get_element_at_index(sensor_list, index);
			sensor_node->sensor_id=sensor_id;
			(sensor_node->counter)++;
			sensor_node->ts=time;
			sensor_node->data_record[(sensor_node->counter)%RUN_AVG_LENGTH]=tmp;//update the arrray
			if(sensor_node->counter<5){sensor_node->avg_tmp=0;}
			else{
				//calculate the average
				double sum=0,avg=0;
				for(int i=0;i<RUN_AVG_LENGTH;i++)
				{
					sum=sum+sensor_node->data_record[i];
				}
				avg=sum/RUN_AVG_LENGTH;
				sensor_node->avg_tmp=avg;

				//if the temperature is exceeding a value
				if((sensor_node->avg_tmp)>SET_MAX_TEMP)
				{
					fprintf(stderr, "room %16u too hot.\n", sensor_node->room_id);
				}
				else if((sensor_node->avg_tmp)<SET_MIN_TEMP)
				{
					fprintf(stderr, "room %16u too cold.\n", sensor_node->room_id);
				}
		   	 }			
				
		   }
		free(sensor_w_node);
		
	}
}

void datamgr_parse_sensor_data(FILE * fp_sensor_map,sbuffer_t ** buffer)
{
	ERROR_HANDLER(fp_sensor_map == NULL, "error");	
	uint16_t sensor_id;
	uint16_t room_id;
	
	//create sensor node list
	sensor_list=dpl_create(element_copy,element_free,element_compare);

	//read data from sensor map to get room id and sensor id and store them in node list
	while(fscanf(fp_sensor_map,"%16hu %16hu \n",&room_id,&sensor_id) !=EOF)
	{
		//create the element of the dplist		
		sensor_data_record_t* sensor_node=malloc(sizeof(sensor_data_record_t));
		//give the value to the node
		sensor_node->sensor_id=sensor_id;
		sensor_node->room_id=room_id;
                dpl_insert_at_index(sensor_list,sensor_node,-1,1);//add the node to the head
		free(sensor_node);
	}
  
	while(1)
	{
		sensor_data_t data;
		int buffer_result;			
		buffer_result=sbuffer_remove(*buffer,&data);
		//condition of termination		
		if(buffer_result==SBUFFER_TIME_OUT){printf("Datamgr close\n");break;}
		if(buffer_result==SBUFFER_SUCCESS)
		{	
		//check if the sensor_id is in the list
		sensor_working_node* sensor_w_node=malloc(sizeof(sensor_working_node));
		sensor_w_node->sensor_id = data.id;

		int index=dpl_get_index_of_element(sensor_list, sensor_w_node);
		if(index==-1){
			char* send=0;
			asprintf(&send,"Received sensor data with invalid sensor node ID %d\n",data.id);
			fifo_send(send);
			free(send);
		}
		else{	
			sensor_data_record_t* sensor_node=(sensor_data_record_t*)dpl_get_element_at_index(sensor_list, index);
			sensor_node->sensor_id=data.id;
			(sensor_node->counter)++;
			sensor_node->ts=data.ts;
			sensor_node->data_record[(sensor_node->counter)%RUN_AVG_LENGTH]=data.value;
			if(sensor_node->counter<5){sensor_node->avg_tmp=0;}
			else{
				//calculate the average
				double sum=0,avg=0;
				for(int i=0;i<RUN_AVG_LENGTH;i++)
				{
					sum=sum+sensor_node->data_record[i];
				}
				avg=sum/RUN_AVG_LENGTH;
				sensor_node->avg_tmp=avg;

				//if the temperature is exceeding a value
				if((sensor_node->avg_tmp)>SET_MAX_TEMP)
				{
					char* send=0;
					asprintf(&send,"The sensor node with <sensorNodeID %d> reports room %d too hot running avg temperature = <%f>\n", sensor_node->sensor_id,sensor_node->room_id,sensor_node->avg_tmp);
					fifo_send(send);	
					free(send);		
				}
				else if((sensor_node->avg_tmp)<SET_MIN_TEMP)
				{
					char* send=0;
					asprintf(&send,"The sensor node with <sensorNodeID %d> reports room %d too cold running avg temperature = <%f>\n", sensor_node->sensor_id,sensor_node->room_id,sensor_node->avg_tmp);
					fifo_send(send);
					free(send);
				}
		   	 }			
				
		   }
		free(sensor_w_node);
		}
		
	}
	
}

void datamgr_free()
{
   dpl_free(&sensor_list, true);
   sensor_list = NULL;//leaks are p
}
    

uint16_t datamgr_get_room_id(sensor_id_t sensor_id)
{
	sensor_working_node* sensor_w_node=malloc(sizeof(sensor_working_node));
	sensor_w_node->sensor_id=sensor_id;
	int index=dpl_get_index_of_element(sensor_list,sensor_w_node);
	if(index==-1){printf("The sensor is not in the list"); return 0;}
	else{
			sensor_data_record_t* n=dpl_get_element_at_index(sensor_list,index);	
			return n->room_id;
		}
}


sensor_value_t datamgr_get_avg(sensor_id_t sensor_id)
{
	sensor_working_node* sensor_w_node=malloc(sizeof(sensor_working_node));
	sensor_w_node->sensor_id=sensor_id;
	int index=dpl_get_index_of_element(sensor_list,sensor_w_node);
	if(index==-1){printf("The sensor is not in the list"); return 0;}
	else{
			sensor_data_record_t* n=dpl_get_element_at_index(sensor_list,index);
			return n->avg_tmp;
		}
	
}


time_t datamgr_get_last_modified(sensor_id_t sensor_id)
{
	sensor_working_node* sensor_w_node=malloc(sizeof(sensor_working_node));
	sensor_w_node->sensor_id=sensor_id;
	int index=dpl_get_index_of_element(sensor_list,sensor_w_node);
	if(index==-1){printf("The sensor is not in the list"); return 0;}
	else{
			sensor_data_record_t* n=dpl_get_element_at_index(sensor_list,index);
			return n->ts;
		}
}


int datamgr_get_total_sensors()
{
	return dpl_size(sensor_list);
}


