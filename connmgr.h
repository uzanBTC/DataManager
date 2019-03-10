#ifndef CONNMGR_H
#define CONNMGR_H

#include "sbuffer.h"

/*
 * This method starts listening on the given port and when when a sensor node connects it 
 * stores the sensor data in the shared buffer.
 */
void connmgr_listen(int port_number, sbuffer_t ** buffer);

/*
 * This method should be called to clean up the connmgr, and to free all used memory. 
 * After this no new connections will be accepted
 */
void connmgr_free();

#endif /* CONNMGR_H */

