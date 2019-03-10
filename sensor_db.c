#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include <sqlite3.h>
#include "sensor_db.h"
#include <unistd.h>
#include <time.h>

DBCONN * init_connection(char clear_up_flag)
{	
	sqlite3 *db;
	char* sql=0;
	char* err_msg=0;
	int try=1;
	int rc;
	while(1)
	{
		rc=sqlite3_open(TO_STRING(DB_NAME),&db);
		if(rc != SQLITE_OK){
			try++;
			if(try>3)
			{
			fprintf(stderr,"Can not open database: %s\n", sqlite3_errmsg(db));
			sqlite3_free(sql);
			sqlite3_free(err_msg);	
			char* send=0;
			asprintf(&send,"Unable to connect to SQL server\n");
			fifo_send(send);
			free(send);	
			return NULL;
			}
			sleep(1);
		
		}
		else
		{
			char* send=0;
			asprintf(&send,"Connection to SQL server established\n");
			fifo_send(send);
			free(send);			
			break;
		}
	}
		
	sql=sqlite3_mprintf("CREATE TABLE IF NOT EXISTS %s(Id INTEGER PRIMARY KEY AUTOINCREMENT, sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);",TO_STRING(TABLE_NAME));
	rc=sqlite3_exec(db,sql,0,0,&err_msg);
	if(rc!=SQLITE_OK)
	{
		fprintf(stderr,"Can not create database: %s\n", sqlite3_errmsg(db));
		sqlite3_free(sql);
		sqlite3_free(err_msg);
		return NULL;
	}

	if(clear_up_flag=='1')
	{
		sql=sqlite3_mprintf("DELETE FROM %s" TO_STRING(TABLE_NAME));		
		rc=sqlite3_exec(db,sql,0,0,&err_msg);
		if(rc!=SQLITE_OK)
		{
			fprintf(stderr,"Cannot clean database: %s\n", sqlite3_errmsg(db));
			sqlite3_free(sql);
			sqlite3_free(err_msg);
			return NULL;
		}
	
	}
	char* send=0;
	asprintf(&send,"New table %s created\n",TO_STRING(TABLE_NAME));
	fifo_send(send);
	free(send);

	sqlite3_free(sql);
	sqlite3_free(err_msg);
	return db;
}

void storagemgr_parse_sensor_data(DBCONN * conn, sbuffer_t ** buffer)
{
	while(1)
	{
		//conn lost fifo
		if(conn==NULL){
			char* send=0;
			asprintf(&send,"Connection to SQL server lost\n");
			fifo_send(send);
			free(send);
		}		
		sensor_data_t data;
		int buffer_result=sbuffer_remove(*buffer,&data);
		if(buffer_result==SBUFFER_TIME_OUT){printf("DataBase close\n");break;}
		if(buffer_result==SBUFFER_SUCCESS)
		{
		  insert_sensor(conn,data.id,data.value,data.ts);
		}

	}

}

void disconnect(DBCONN *conn)
{
	sqlite3_close(conn);
	
}

int insert_sensor(DBCONN * conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
    char *err_msg = 0;
    char *sql = sqlite3_mprintf("INSERT INTO %s VALUES(NULL,%d,%lf,%ld);",TO_STRING(TABLE_NAME),id,value,ts);
    int rc = sqlite3_exec(conn, sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "Can not insert: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
	sqlite3_free(sql);
        
        return 1;
    } 
    sqlite3_free(err_msg); 
	sqlite3_free(sql);
    
    return 0;
}

int insert_sensor_from_file(DBCONN * conn, FILE * sensor_data)
{
	char *err_msg=0;	
	sensor_id_t sensor_id; 
	sensor_value_t value;  
	sensor_ts_t ts;

	while(fread(&sensor_id,sizeof(uint16_t),1,sensor_data))
	{
		fread(&value,sizeof(double),1,sensor_data);
		fread(&ts,sizeof(time_t),1,sensor_data);
		int flag=insert_sensor(conn,sensor_id,value,ts);
		if(flag==1){
			fprintf(stderr, "Can not insert from file: %s\n",err_msg);  
        		sqlite3_free(err_msg);
			return 1;
		}
	}
	sqlite3_free(err_msg);
	return 0;
}

int find_sensor_all(DBCONN * conn, callback_t f)
{
	char *err_msg=0;
	char *sql=sqlite3_mprintf("SELECT * FROM %s;",TO_STRING(TABLE_NAME));  
	int rc=sqlite3_exec(conn, sql, f, 0, &err_msg);
	if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "Can not find all data: %s\n", err_msg);  
        sqlite3_free(err_msg);        
		sqlite3_free(sql);  
        return 1;
       } 
    sqlite3_free(err_msg); 
	sqlite3_free(sql);
    
    return 0;
}

int find_sensor_by_value(DBCONN * conn, sensor_value_t value, callback_t f)
{
	char *err_msg=0;
	char *sql=sqlite3_mprintf("SELECT * FROM %s WHERE sensor_value=%lf;",TO_STRING(TABLE_NAME),value);
	int rc=sqlite3_exec(conn, sql, f, 0, &err_msg);
	if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "Can not find data by value: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
		sqlite3_free(sql);
        
        return 1;
       } 
    sqlite3_free(err_msg); 
	sqlite3_free(sql);
    
    return 0;
}


int find_sensor_exceed_value(DBCONN * conn, sensor_value_t value, callback_t f)
{
	char *err_msg=0;
	char *sql=sqlite3_mprintf("SELECT * FROM %s WHERE sensor_value>%lf;",TO_STRING(TABLE_NAME),value);
	int rc=sqlite3_exec(conn, sql, f, 0, &err_msg);
	if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "Can not find data: %s\n", err_msg);       
        sqlite3_free(err_msg);        
		sqlite3_free(sql);
        
        return 1;
       } 
    sqlite3_free(err_msg); 
	sqlite3_free(sql);
    
    return 0;
}


int find_sensor_by_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
	char *err_msg=0;
	char *sql=sqlite3_mprintf("SELECT * FROM %s WHERE timestamp=%d;",TO_STRING(TABLE_NAME),ts);
	int rc=sqlite3_exec(conn, sql, f, 0, &err_msg);
	if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "can not find data by ts: %s\n", err_msg);       
        sqlite3_free(err_msg);        
	sqlite3_free(sql);
        
        return 1;
       } 
	sqlite3_free(err_msg); 
	sqlite3_free(sql);
    
    return 0;
}


int find_sensor_after_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
	char *err_msg=0;
	char *sql=sqlite3_mprintf("SELECT * FROM %s WHERE timestamp>%d;",TO_STRING(TABLE_NAME),ts);
	int rc=sqlite3_exec(conn, sql, f, 0, &err_msg);
	if (rc != SQLITE_OK ) {
        
        fprintf(stderr, "Can not find data %s\n", err_msg);       
        sqlite3_free(err_msg);        
		sqlite3_free(sql);
        
        return 1;
       } 
    sqlite3_free(err_msg); 
	sqlite3_free(sql);
    
    return 0;
}
