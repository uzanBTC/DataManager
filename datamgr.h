#ifndef DATAMGR_H
#define DATAMGR_H

#include <time.h>
#include <stdio.h>
#include "config.h"
#include "sbuffer.h"
#include "lib/dplist.h"

#ifndef RUN_AVG_LENGTH
  #define RUN_AVG_LENGTH 5
#endif

#ifndef SET_MAX_TEMP
  #error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
  #error SET_MIN_TEMP not set
#endif

/*
 * Use ERROR_HANDLER() for handling memory allocation problems, invalid sensor IDs, non-existing files, etc.
 */
#define ERROR_HANDLER(condition,...) 	do { \
					  if (condition) { \
					    printf("\nError: in %s - function %s at line %d: %s\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)

/*
 *  This method holds the core functionality of your datamgr. It takes in 2 file pointers to the sensor files and parses them. 
 *  When the method finishes all data should be in the internal pointer list and all log messages should be printed to stderr.
 *  REMARK: this is the function you already implemented in a previous lab but for the final assignment you read data from the shared buffer and not from file. That's why you find   *  below the modified function 'datamgr_parse_sensor_data'
 */

void datamgr_parse_sensor_files(FILE * fp_sensor_map, FILE * fp_sensor_data);

/*
 * Reads continiously all data from the shared buffer data structure, parse the room_id's
 * and calculate the running avarage for all sensor ids
 * When *buffer becomes NULL the method finishes. This method will NOT automatically free all used memory
 */
void datamgr_parse_sensor_data(FILE * fp_sensor_map, sbuffer_t ** buffer);


/*
 * We prefer a solution based on 1 sbuffer, but if you can't implement such a solution and you use 2 sbuffers. You should use the following function header in that situation:
 */
// void datamgr_parse_sensor_data(FILE * fp_sensor_map, sbuffer_t ** buffer1, sbuffer_t ** buffer2);


/*
 * This method should be called to clean up the datamgr, and to free all used memory. 
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified 
 * or datamgr_get_total_sensors will not return a valid result
 */
void datamgr_free();

/*
 * Gets the room ID for a certain sensor ID
 */
uint16_t datamgr_get_room_id(sensor_id_t sensor_id);

/*
 * Gets the running AVG of a certain senor ID 
 */
sensor_value_t datamgr_get_avg(sensor_id_t sensor_id);

/*
 * Returns the time of the last reading for a certain sensor ID
 */
time_t datamgr_get_last_modified(sensor_id_t sensor_id);

/*
 * Return the total amount of unique sensor ID's recorded by the datamgr
 */
int datamgr_get_total_sensors();

#endif /* DATAMGR_H */

