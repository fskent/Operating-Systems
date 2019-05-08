#include "networkdriver.h"
#include <pthread.h>
#include "BoundedBuffer.h"
#include "freepacketdescriptorstore__full.h"
#include <stdio.h>

/* 
	Steve Kent. fkent. CIS 415 P2.
	This is my own work except for the following acknowledgements. I would like to give
	credit to Roscoe Casita for the pseudo-code and diagrams from labs. These were very
	helpful to my understanding of the project. My send and receive threads follow the
	general ideas put forth in the labs. 
 */

#define POOL 6
#define SEND 6
#define REC 24
static FreePacketDescriptorStore *fpds;
static NetworkDevice *_nd;
static BoundedBuffer *rec_buf;
static BoundedBuffer *send_buf;
static BoundedBuffer *pool;
pthread_t send_thread;
pthread_t rec_thread;


/*
Helper function used to get PD from pool or FPDS. Returns 0 on failure. 
Non-Blocking. 
*/
int nb_get_pd(BoundedBuffer *pool, FreePacketDescriptorStore *fpds, PacketDescriptor *pd){
	if (pool->nonblockingRead(pool, (void **)pd) != 1){
		if (fpds->nonblockingGet(fpds, pd) != 1){
			return 0;
		}
	}
	return 1;
}

/*
Helper function used to place PD in pool or FPDS. Prints error on failure. 
Non-Blocking.
*/
void nb_put_pd(BoundedBuffer *pool, FreePacketDescriptorStore *fpds, PacketDescriptor *pd){
	if (pool->nonblockingWrite(pool, pd) != 1){
		if (fpds->nonblockingPut(fpds, pd) != 1){
			printf("error\n");
		}
	}
}

/* 
The sender thread is in charge of pulling PD's from the send buffer
and sending those packets using the network device. Given 5 attempts
to send. After sent, packets are returned to pool or FPDS. 
*/
void *sender(){
	PacketDescriptor **pd;
	int i;
	while(1){
		send_buf->blockingRead(send_buf, (void **)&pd);
		for (i = 0; i < 5; i++){
			_nd->sendPacket(_nd, *pd);
		}
		nb_put_pd(pool, fpds, *pd);
	}
}

/*
The receiver thread is in charge of registering PD's and waiting for the 
network device to fill packets with data. This thread then pulls from 
the pool or FPDS and writes packets to the receive buffer. Failed attempts 
are returned to pool or FPDS. 
*/
void *receiver(){
	PacketDescriptor *pd;	
	fpds->blockingGet(fpds, &pd);
	initPD(pd);
	_nd->registerPD(_nd, pd);
	while(1){
		_nd->awaitIncomingPacket(_nd);
		if ((nb_get_pd(pool, fpds, pd) == 1)){
			if ((rec_buf->nonblockingWrite(rec_buf, pd)) == 0){
				nb_put_pd(pool, fpds, pd);
			}	
		}
		initPD(pd);
		_nd->registerPD(_nd, pd);
	}
}

/* 
Simply write to send buffer. 
Blocking.
*/
void blocking_send_packet(PacketDescriptor *pd){
	send_buf->blockingWrite(send_buf, pd);
	return;
}

/*
Simply write to send buffer.
Non-Blocking. Returns 1 on success. 
*/
int  nonblocking_send_packet(PacketDescriptor *pd){
	if (send_buf->nonblockingWrite(send_buf, pd)){
		return 1;
	}
	return 0;
}

/* These calls hand in a PacketDescriptor for dispatching */
/* The nonblocking call must return promptly, indicating whether or */
/* not the indicated packet has been accepted by your code          */
/* (it might not be if your internal buffer is full) 1=OK, 0=not OK */
/* The blocking call will usually return promptly, but there may be */
/* a delay while it waits for space in your buffers.                */
/* Neither call should delay until the packet is actually sent      */


/*
Read from the receive buffer.
If the PID's match then return.
Blocking.
*/
void blocking_get_packet(PacketDescriptor **pd, PID pid){
	rec_buf->blockingRead(rec_buf, (void **)pd);
	if (getPID(*pd) == pid){
		return;
	}
}

/*
Read from the receive buffer.
If the PID's match then return 1
Non-Blocking.
*/

int  nonblocking_get_packet(PacketDescriptor **pd, PID pid){

	if (rec_buf->nonblockingRead(rec_buf, (void **)pd)){
		if (getPID(*pd) == pid){
			return 1;
		}
	}	
	return 0;
}

/* These represent requests for packets by the application threads */
/* The nonblocking call must return promptly, with the result 1 if */
/* a packet was found (and the first argument set accordingly) or  */
/* 0 if no packet was waiting.                                     */
/* The blocking call only returns when a packet has been received  */
/* for the indicated process, and the first arg points at it.      */
/* Both calls indicate their process number and should only be     */
/* given appropriate packets. You may use a small bounded buffer   */
/* to hold packets that haven't yet been collected by a process,   */
/* but are also allowed to discard extra packets if at least one   */
/* is waiting uncollected for the same PID. i.e. applications must */
/* collect their packets reasonably promptly, or risk packet loss. */

void init_network_driver(NetworkDevice *nd, void *mem_start, unsigned long mem_length, FreePacketDescriptorStore **fpds_ptr){
		                 
	fpds = FreePacketDescriptorStore_create(mem_start, mem_length);
	*fpds_ptr = fpds;	              
    _nd = nd;                     
    rec_buf = BoundedBuffer_create(REC);
	send_buf = BoundedBuffer_create(SEND);
    pool = BoundedBuffer_create(POOL);

	pthread_create(&send_thread, NULL, sender, NULL);
	pthread_create(&rec_thread, NULL, receiver, NULL);
	                                 
}
/* Called before any other methods, to allow you to initialise */
/* data structures and start any internal threads.             */ 
/* Arguments:                                                  */
/*   nd: the NetworkDevice that you must drive,                */
/*   mem_start, mem_length: some memory for PacketDescriptors  */
/*   fpds_ptr: You hand back a FreePacketDescriptorStore into  */
/*             which you have put the divided up memory        */
/* Hint: just divide the memory up into pieces of the right size */
/*       passing in pointers to each of them                     */
