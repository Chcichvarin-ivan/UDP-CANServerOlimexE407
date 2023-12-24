/*
 * udp_server.c
 *
 *  Created on: Dec 20, 2023
 *      Author: Ivan Chichvarin
 */


#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/udp.h"
#include "udp_server.h"

//Define the port number
#define UDP_SERVER_PORT    7   /* define the UDP local connection port */
#define UDP_CLIENT_PORT    7   /* define the UDP remote connection port */

//Declare the receive data callback function, specified in the initialization function
void udp_server_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);


//UDP server initialization function
void udp_server_init(void)
{
   struct udp_pcb *upcb;
   err_t err;

   /* Create a new UDP control block  */
   upcb = udp_new();  //Create a new UDP control block

   if (upcb)
   {
     /* Bind the upcb to the UDP_PORT port */
     /* Using IP_ADDR_ANY allow the upcb to be used by any local interface */
      err = udp_bind(upcb, IP_ADDR_ANY, UDP_SERVER_PORT);   //Bind local IP address and port

      if(err == ERR_OK)
      {
        /* Set a receive callback for the upcb */
        udp_recv(upcb, udp_server_receive_callback, NULL);   //Register callback function for receiving data
      }
      else
      {
        udp_remove(upcb);
      }
   }
}

//Data callback function
void udp_server_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{

	struct pbuf *q;
	if(p!=NULL)	//in case there is an error in processing the packet
	{
		for(q=p;q!=NULL;q=q->next)
	    {
	    	if(p->len < UDP_MAX_MSG_SIZE)
	    	{
	    		udp_server_push_packet(addr,port,q);
	    	}
	    }
		/* Tell the client that we have accepted it */
		udp_sendto(upcb, p,addr,port);  //Echo data

		/* Free the p buffer */
		pbuf_free(p);
	}else
	{
		udp_disconnect(upcb);

	}
}

void udp_server_send(udp_message_type msg_to_send)
{

}
