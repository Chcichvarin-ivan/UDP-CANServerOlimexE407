/*
 * udp_server.c
 *
 *  Created on: Dec 20, 2023
 *      Author: Ivan Chichvarin
 */


#include "udp_server.h"
#include "string.h"

//Define the port number
#define UDP_SERVER_PORT    7   /* define the UDP local connection port */
#define UDP_CLIENT_PORT    7   /* define the UDP remote connection port */

struct udp_pcb *udp_pcb;
extern osMessageQueueId_t UdpRxPacketQueueHandle;
extern osMessageQueueId_t UdpTxPacketQueueHandle;
//Declare the receive data callback function, specified in the initialization function
void udp_server_receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);


//UDP server initialization function
void udp_server_init(void)
{

   err_t err;

   /* Create a new UDP control block  */
   udp_pcb = udp_new();  //Create a new UDP control block

   if (udp_pcb)
   {
     /* Bind the upcb to the UDP_PORT port */
     /* Using IP_ADDR_ANY allow the upcb to be used by any local interface */
      err = udp_bind(udp_pcb, IP_ADDR_ANY, UDP_SERVER_PORT);   //Bind local IP address and port

      if(err == ERR_OK)
      {
        /* Set a receive callback for the upcb */
        udp_recv(udp_pcb, udp_server_receive_callback, NULL);   //Register callback function for receiving data
      }
      else
      {
        udp_remove(udp_pcb);
      }
   }
}

void udp_handler(void)
{
	udp_message_type in_msg;
	udp_message_type out_msg;


	if(osOK == osMessageQueueGet(UdpRxPacketQueueHandle,&in_msg,0,0))
	{//process udp message

	}

	if(osOK == osMessageQueueGet(UdpTxPacketQueueHandle,&out_msg,0,0))
	{
		udp_server_send(out_msg);
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
		  //Echo data

		/* Free the p buffer */
		pbuf_free(p);
	}else
	{
		udp_disconnect(upcb);

	}
}

void udp_server_send(udp_message_type msg_to_send)
{
	ip_addr_t dest_addr;
	dest_addr.addr = msg_to_send.ip_addr;

	struct pbuf *p_out;
	p_out = pbuf_alloc(PBUF_TRANSPORT,msg_to_send.length,PBUF_POOL);
	pbuf_take(p_out, msg_to_send.udp_recvbuf,msg_to_send.length);
	udp_sendto(udp_pcb,p_out,&dest_addr,msg_to_send.port);
	pbuf_free(p_out);
}

bool udp_server_push_packet(const ip_addr_t *in_addr,u16_t in_port,struct pbuf *in_buf)
{
	bool ret_val = false;
	osStatus_t is_enqueued;
	udp_message_type in_msg;
	in_msg.ip_addr = in_addr->addr;
	in_msg.port = in_port;
	in_msg.length = in_buf->len;
	memset(in_msg.udp_recvbuf,0,UDP_MAX_MSG_SIZE);
	memcpy(in_msg.udp_recvbuf,in_buf->payload,in_buf->len);
	is_enqueued  = osMessageQueuePut(UdpRxPacketQueueHandle,&in_msg,0,osWaitForever);
	if(is_enqueued == osOK)
	{
		ret_val = true;
	}else
	{
		ret_val = false;
	}
	return ret_val;
}
