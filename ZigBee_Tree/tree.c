/* */
#include "contiki.h"
#include "net/rime.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <math.h>
#include <stdio.h>

#define CHANNEL 135
//variable globle decleration
//c is the Cm, r is the Rm, and l is the Lm.
int c,r,l; 
//the variable (addrrang) bellow is reffere to "address rang", which is the number of address that this node have to assign to it's children. 
int addrrang =0; 
// every node has three address sigments to assing to three children, the varialbe (addrsgmentsgiven) bellow is reffere to how many of these signments been assigned, so when this varible = 3 that means this node has three children and cannot give any more addresses.
int addrsgmentsgiven = 0; 
// (eachsigment)  represent the numbder of addresses that each child for this will have.
int eachsigment = 0;
// (mystart) is the first address of the address range that this node have, and it is the same as it's owne address.
// (myend) is the last address of the address range that this node have.
int mystart, myend;
// (parent) is the address of the parent of this node.
rimeaddr_t parent;
//(child[]) is an array hold the addresses of all children of this node.
rimeaddr_t child[3] = {NULL,NULL,NULL};
/*-------------------------------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------------------------------------------*/
/* The un-cast connection and it's call back methods */
/*---------------------------------------------------*/
/* this method called when there is another node that assigning address to this node, the sender will be the parent */
static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
  //printing that it's recived a uni-cast messsage and the sender address
  printf("unicast message received from %d.%d\n", from->u8[0], from->u8[1]);
  //for copying the "from" address to the parent, this method didnot work "rimeaddr_copy (&parent , &from );" so I did that manually by wirting the next two line. 
  parent.u8[0] = from->u8[0];
  parent.u8[1] = from->u8[1];
  // getting the buffer value (the array).
  int *xx = (int *)packetbuf_dataptr();
  // assigning the Cm,Rm and Lm values.
  c= xx[0];
  r= xx[1];
  l= xx[2];
  //setting the first and the last addresses in the range specified of this node.
  mystart = xx[3];
  myend = xx[4];
  //calculating the address range of this node.
  addrrang = myend - mystart ;

  //printing the old and the new address of this node.
  printf("the node %d.%d been given a new address, the new address is %d,%d \n",rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1],0,mystart);
  //setting the new address.
  rimeaddr_t newfirstaddr;
  newfirstaddr.u8[0] = 0;
  newfirstaddr.u8[1] = mystart;
  rimeaddr_set_node_addr ( &newfirstaddr );
}
/*----------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
/*-------------------------------------------------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------------------------------------------------*/
/* ibc connection and it's call back methods */
/*-------------------------------------------*/
/* This method called when the node recive a broadcast message (JOIN_REQ message), it will send a uni-cast message with the addresses to the sender if it has addresses or it will ignore the broadcast if it don't have addresses to assign*/
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
  // if it has been join the tree and has address range to give from.
  if (addrrang != 0 ){
		//if it dosen't have three children yet.
		if (addrsgmentsgiven < 3){
			
			//calculate the number of adresse range that it can give to any of it's children.
			eachsigment = addrrang / 3 ;

			/* calculating the node range for this child. i have used a formula to calculate the address range for each child, the formula is based on starting from the node currentaddress, increment that by one (which the current address so it don't be within the range) then calculate the addresses that allready been given from the child number (which is addrsgmentsgiven) and the number of addresses in each sigment (eachsigment) */
			int start = eachsigment * addrsgmentsgiven + 1 + mystart ;
			int end = eachsigment * addrsgmentsgiven + eachsigment + mystart ;

			//save that the node that I'm gong to assign the addresses to is this node's child.
			rimeaddr_copy (&child[addrsgmentsgiven] , &from );

			//one sigment out of three has been assigned.
			addrsgmentsgiven += 1; 

			// saveing all the information that i want to send into an array and then send it using the uni-cast connection.
			int infoarray[] = {c,r,l,start,end}; 
			packetbuf_copyfrom(infoarray , sizeof(infoarray));
			unicast_send(&uc, from);  // this is the JOIN_CONF message.	
	 	}		
	}	
}
/*-------------------------------------------*/
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*-------------------------------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------------------------------------------*/
/** The mh connection  */
static struct multihop_conn multihop;

/*---------------------------------------------------------------------------*/
PROCESS(example_multihop_process, "multihop example");
AUTOSTART_PROCESSES(&example_multihop_process);
/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/
/* this method called when arriving the message ti it's destination */
static void
recv(struct multihop_conn *c, const rimeaddr_t *sender,
     const rimeaddr_t *prevhop,
     uint8_t hops)
{
  //print the sender address, the reciver address (this node address) and the message 
  printf("message arrived from %d.%d, my address is %d.%d, and the message is '%s'\n", sender->u8[0],sender->u8[1],rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1], (char *)packetbuf_dataptr());
}

/*---------------------------------------------------------------------------*/
/* this is the forwarding message, it called when reciving message been sent to other node */
static rimeaddr_t *
forward(struct multihop_conn *c,
	const rimeaddr_t *originator, const rimeaddr_t *dest,
	const rimeaddr_t *prevhop, uint8_t hops)
{
  // copying the dest address
  rimeaddr_t tempaddr;
  tempaddr = *dest;

  // checking it the recived message is out of this node range, then it forward it to it's parent
  if ( (tempaddr.u8[1] < mystart) || (tempaddr.u8[1] > myend) ) {
	printf("I am node %d.%d and I'm Forwarding packet to my parent %d.%d \n", rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], parent.u8[0], parent.u8[1]);
	return &parent;
  }
  else{
	// checking it the recived message is withing the first sigment of it's address range then forward it to it's first child.
	if ( (tempaddr.u8[1] > mystart) && ( tempaddr.u8[1] <= ((eachsigment * 0) + eachsigment + mystart) ) ) {
		// checking it the first child exist? 		
		if (!rimeaddr_cmp(&child[0], NULL)){
			printf("I am node %d.%d and I'm Forwarding packet to my first child %d.%d \n", rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], child[0].u8[0], child[0].u8[1]);
			return &child[0];
		}
		// if the first child not exist.
		else{
			printf("ERROR: for unknown destination! ");
			return NULL;
		}
	}
	// checking it the recived message is withing the second sigment of it's address range then forward it to it's second child.
	else if ((tempaddr.u8[1] >= ((eachsigment * 1) + 1 + mystart)) && (tempaddr.u8[1] <= ((eachsigment * 1) + eachsigment + mystart))) {
		// checking it the second child exist? 				
		if (!rimeaddr_cmp(&child[1], NULL)){
			printf("I am %d.%d and I'm Forwarding packet to my second child %d.%d \n", rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], child[1].u8[0], child[1].u8[1]);
			return &child[1];
		}
		// if the second child not exist
		else{
			printf("ERROR: for unknown destination! ");
			return NULL;
		}
	}
		// checking it the recived message is withing the third sigment of it's address range then forward it to it's third child.
		else if (tempaddr.u8[1] >= ((eachsigment * 2) + 1 + mystart)) {
			// checking it the third child exist?
			if (!rimeaddr_cmp(&child[2], NULL)) {
				printf("I am %d.%d and I'm Forwarding packet to my third child %d.%d \n", rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1], child[2].u8[0], child[2].u8[1]);
			return &child[2];
			}
			// if the third child not exist
			else{
				printf("ERROR: for unknown destination! ");
				return NULL;
			}
		}			
  }
}
/*---------------------------------------------------------------------------*/
static const struct multihop_callbacks multihop_call = {recv, forward};
/*-------------------------------------------------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------------------------------------------------*/
PROCESS_THREAD(example_multihop_process, ev, data)
{
  PROCESS_EXITHANDLER(multihop_close(&multihop);)
    
  PROCESS_BEGIN();
 
  //opening the connection (mh, uni-cast and broadcast)
  multihop_open(&multihop, CHANNEL, &multihop_call);
  unicast_open(&uc, 146, &unicast_callbacks);
  broadcast_open(&broadcast, 129, &broadcast_call);
  
  // the first address to compare to
  rimeaddr_t firstaddr;
  firstaddr.u8[0] = 1;
  firstaddr.u8[1] = 0;

  // the address to assign to the coordinator
  rimeaddr_t newfirstaddr;
  newfirstaddr.u8[0] = 0;
  newfirstaddr.u8[1] = 0;

  // if this node has the address 1.0 by contiki it is my corrdinator so change it to address 0.0
  if(rimeaddr_cmp(&firstaddr, &rimeaddr_node_addr)) {
	rimeaddr_set_node_addr ( &newfirstaddr );
	printf("first node address setted to %d.%d \n",rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1]);
        mystart = (int) rimeaddr_node_addr.u8[1] ;   //set the value of mystart variable for the first node.
  }	         

  /* if this node is the coordinator set l.c,r and count the addrees rang*/ 	     
  if(rimeaddr_cmp(&newfirstaddr, &rimeaddr_node_addr)){
	l= 5;
	r= 3;
	c= 3;

	/* count address rang, note:-I have implement the pow() function by my self as it didn't work with me.*/
	int i, j;
	int powe = 1;
	addrrang += 1;
	for(i = 1; i < l+1 ; i++) {
		powe = 1;
		for (j=1; j< i+1; j++)
			powe *= r;
		addrrang += powe ;
	}
	printf("the total calculated address range %d \n",addrrang);
	addrrang -= 1; //  minus the address range by one which is the address given to the root 0.0
  }

  /* if this node is not the coordinator send announcment (as JOIN_REQ) */ 
  if(!rimeaddr_cmp(&newfirstaddr, &rimeaddr_node_addr)) {
	while(addrrang == 0 ) {
			/* Delay 1-2 seconds */
			static struct etimer et; 
			etimer_set(&et, CLOCK_SECOND * 1 + random_rand() % (CLOCK_SECOND * 1));
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

			packetbuf_copyfrom("JOIN_REQ", 8);
			broadcast_send(&broadcast);
	}
   }


  /* Activate the button sensor. We use the button to drive traffic -
     when the button is pressed, a packet is sent. */
  SENSORS_ACTIVATE(button_sensor);

  /* Loop forever, send a packet when the button is pressed. */
  while(1) {
    rimeaddr_t to;

    /* Wait until we get a sensor event with the button sensor as data. */
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
			     data == &button_sensor);

    /* Copy the "Hello" to the packet buffer. */
    packetbuf_copyfrom("Hello", 6);

    /* Set the Rime address of the final receiver of the packet to 0.0. */
    int r = rand() % 20;
    to.u8[0] = 0;
    to.u8[1] = r;

    /* Send the packet. */
    multihop_send(&multihop, &to);

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
