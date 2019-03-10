//#define _SVID_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "config.h"
#include "lib/tcpsock.h"

// conditional compilation option to control the number of measurements this sensor node wil generate
#if (LOOPS > 1)
  #define UPDATE(i) (i--)
#else
  #define LOOPS 1 
  #define UPDATE(i) (void)0 //create infinit loop
#endif

// conditional compilation option to log all sensor data to a text file
#ifdef LOG_SENSOR_DATA

  #define LOG_FILE	"sensor_log"

  #define LOG_OPEN()						\
    FILE *fp_log; 						\
    do { 							\
      fp_log = fopen(LOG_FILE, "w");				\
      if ((fp_log)==NULL) { 					\
	printf("%s\n","couldn't create log file"); 		\
	exit(EXIT_FAILURE); 					\
      }								\
    } while(0)

  #define LOG_PRINTF(sensor_id,temperature,timestamp)							\
      do { 												\
	fprintf(fp_log, "%" PRIu16 " %g %ld\n", (sensor_id), (temperature), (long int)(timestamp));	\
	fflush(fp_log);											\
      } while(0)
      
  #define LOG_CLOSE()	fclose(fp_log);	     

#else
  #define LOG_OPEN(...) (void)0
  #define LOG_PRINTF(...) (void)0
  #define LOG_CLOSE(...) (void)0
#endif

#define INITIAL_TEMPERATURE 	16
#define TEMP_DEV 		5	// max afwijking vorige temperatuur in 0.1 celsius


void print_help(void);

/*
 * argv[1] = sensor ID
 * argv[2] = sleep time
 * argv[3] = server IP
 * argv[4] = server port
 * argv[5] = right(0) or cold(1) data or hot(2) data
 * argv[6] = connection time
 */

int main( int argc, char *argv[] )
{
  sensor_data_t	data;
	time_t  current_time, last_connect;
	time(&last_connect);
  int	server_port;
  int	mode, conn_time;
  char server_ip[] = "000.000.000.000"; 
  tcpsock_t * client;
  int i, bytes,sleep_time;
  
  LOG_OPEN();
  
  if (argc != 7)
  {
    print_help();
    exit(EXIT_SUCCESS);
  }
  else
  {
    // to do: user input validation!
    data.id = atoi(argv[1]);
    sleep_time = atoi(argv[2]);
    strncpy(server_ip, argv[3],strlen(server_ip));
    server_port = atoi(argv[4]);
	mode=atoi(argv[5]);
	conn_time=atoi(argv[6]);
  }
  
  srand48( time(NULL) );
  
  // open TCP connection to the server; server is listening to SERVER_IP and PORT
  if (tcp_active_open(&client,server_port,server_ip )!=TCP_NO_ERROR) exit(EXIT_FAILURE);
  data.value = INITIAL_TEMPERATURE; 
  i=LOOPS;
  while(i) 
  {
    time(&current_time);
	if(current_time-last_connect>conn_time)
		break;
	//if(mode==0)
		//data.value = data.value + TEMP_DEV * ((drand48() - 0.5)/10); 
	switch( mode ) 
	{
		case 0:
			data.value = data.value + TEMP_DEV * ((drand48() - 0.5)/10);
			break;
		case 1:
			data.value=3;
			break;
		case 2:
			data.value=99;
			break;
		default:
			data.value = data.value + TEMP_DEV * ((drand48() - 0.5)/10); 
	}	
    time(&data.ts);
    // send data to server in this order (!!): <sensor_id><temperature><timestamp>
    // remark: don't send as a struct!
    bytes = sizeof(data.id);
    if (tcp_send( client,(void *)&data.id,&bytes)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
    bytes = sizeof(data.value);
    if (tcp_send(client,(void *)&data.value,&bytes)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
    bytes = sizeof(data.ts);
    if (tcp_send(client,(void *)&data.ts,&bytes)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
    LOG_PRINTF(data.id,data.value,data.ts);
    sleep(sleep_time);
    UPDATE(i);
  }
  
  if (tcp_close( &client )!=TCP_NO_ERROR) exit(EXIT_FAILURE);
  
  LOG_CLOSE();
  
  exit(EXIT_SUCCESS);
}
  
  
void print_help(void)
{
  printf("Use this program with 6 command line options: \n");
  printf("\t%-15s : a unique sensor node ID\n", "\'ID\'");
  printf("\t%-15s : node sleep time (in sec) between two measurements\n","\'sleep time\'");
  printf("\t%-15s : TCP server IP address\n", "\'server IP\'");
  printf("\t%-15s : TCP server port number\n", "\'server port\'");
  printf("\t%-15s : TCP client send right or wrong data\n", "\'MODE\'");
  printf("\t%-15s : TCP client connect time \n", "\'connect time\'");
}
