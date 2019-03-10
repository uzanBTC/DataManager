# DataManager
C, Linux. TCP/IP, multithreading, multiprocessing, MySQL

How to use those C files
connmgr.c: using polling listening sockets; Connection to TCP clients and receive data.
datamgr.c: detect abnormal data, give alerts and write them to log file.
sensor_db.c: put all received data to MySql Database
main.c: creating three threads for running connmgr.c, datamgr.c, sensor_db.c. And creating a child process to write log file. 
        The communication between main process and child process is FIFO
