#define __DEFAULT_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>


#define FILE_ERROR(fp,error_msg) 	do { \
					  if ((fp)==NULL) { \
					    printf("%s\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)
	
					
#define NUM_MEASUREMENTS 50
#define SLEEP_TIME	30	// every SLEEP_TIME seconds, sensors wake up and measure temperature
#define NUM_SENSORS	8	// also defines number of rooms (currently 1 room = 1 sensor)
#define TEMP_DEV 	5	// max afwijking vorige temperatuur in 0.1 celsius

uint16_t room_id[NUM_SENSORS] = {1,2,3,4,11,12,13,14};
uint16_t sensor_id[NUM_SENSORS] = {15,21,37,49,112,129,132,142};
double sensor_temperature[NUM_SENSORS] = {15,17,18,19,20,23,24,25}; // starting temperatures

int main( int argc, char *argv[] )
{
  FILE * fp_text, * fp_bin;
  int i, j;
  time_t starttime = time(&starttime);
  srand48( time(NULL) );
  
  // generate ascii file room_sensor.map
  fp_text = fopen("room_sensor.map", "w");
  FILE_ERROR(fp_text,"Couldn't create room_sensor.map\n");
  for( i=0; i<NUM_SENSORS; i++)
  {
    fprintf(fp_text,"%" PRIu16 " %" PRIu16 "\n", room_id[i], sensor_id[i]); 
  }
  fclose(fp_text);
 
  // generate binary file sensor_data and corresponding log file
  fp_bin = fopen("sensor_data", "w");
  FILE_ERROR(fp_bin,"Couldn't create sensor_data\n");  

  #ifdef DEBUG // save sensor data also in text format for test purposes
    fp_text = fopen("sensor_data_text", "w");
    FILE_ERROR(fp_text,"Couldn't create sensor_data in text\n");
  #endif

  for( i=0; i<NUM_MEASUREMENTS; i++, starttime+=SLEEP_TIME ) 
  {
    for( j=0; j<NUM_SENSORS; j++)
    {
       // write current temperatures to file
      fwrite(sensor_id+j, sizeof(sensor_id[0]), 1, fp_bin);
      fwrite(&(sensor_temperature[j]), sizeof(sensor_temperature[0]), 1, fp_bin);
      fwrite(&starttime, sizeof(time_t), 1, fp_bin);      
      #ifdef DEBUG
	fprintf(fp_text,"%" PRIu16 " %g %ld\n", sensor_id[j],sensor_temperature[j],(long)starttime);    
      #endif      
      
      // get new temperature: still needs some fine-tuning ...
      sensor_temperature[j] = sensor_temperature[j] + TEMP_DEV * ((drand48() - 0.5)/10); 
    }
  }
  
  fclose(fp_bin);
  #ifdef DEBUG
    fclose(fp_text);
  #endif
  
  return 0;
}

