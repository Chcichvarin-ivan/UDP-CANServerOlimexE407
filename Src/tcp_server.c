/*
 * tcp_server.c
 *
 *  Created on: Apr 1, 2023
 *      Author: Ivan Chichvarin
 */




#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"


#if LWIP_TCP

static struct tcp_pcb *tcp_echoserver_pcb;

/* ECHO protocol states */
enum tcp_echoserver_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

/* structure for maintaing connection infos to be passed as argument
   to LwIP callbacks*/
struct tcp_echoserver_struct
{
  u8_t state;             /* current connection state */
  u8_t retries;
  struct tcp_pcb *pcb;    /* pointer on the current tcp_pcb */
  struct pbuf *p;         /* pointer on the received/to be transmitted pbuf */
};


static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_echoserver_error(void *arg, err_t err);
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_echoserver_send(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es);
static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es);


/**
  * @brief  Initializes the tcp echo server
  * @param  None
  * @retval None
  */
void tcp_echoserver_init(void)
{
  /* create new tcp pcb */
  tcp_echoserver_pcb = tcp_new();
  // The memory block is dynamically assigned, and it is necessary to judge whether it is created.
  if (tcp_echoserver_pcb != NULL)
  {
    err_t err;

    /*
    	     For the TCP control block we created, bind the first IP of this unit, and port 7
    */
    err = tcp_bind(tcp_echoserver_pcb, IP_ADDR_ANY, 7);

    if (err == ERR_OK)
    {
      /*
      	       Open monitor
      	       Monitor return value is a new TCP control block. If you listen success, the previous control block will release it.
      */
      tcp_echoserver_pcb = tcp_listen(tcp_echoserver_pcb);

      /*
      	       Waiting for client connection
      	       This is implemented in the form of a callback, that is, TCP_ECHOSERVER_ACCEPT
      */
      tcp_accept(tcp_echoserver_pcb, tcp_echoserver_accept);
    }
    else
    {
      /*
      	       If the binding failed, release the TCP control block
      */
      memp_free(MEMP_TCP_PCB, tcp_echoserver_pcb);
    }
  }
}
/**
  * @brief  This function is the implementation of tcp_accept LwIP callback
  * @param  arg: not used
     * @Param newPCB: Point to the control block of the client's TCP
  * @param  err: not used
  * @retval err_t: error status
  */
static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  // This is the entire Echoserver control block (structure)
  struct tcp_echoserver_struct *es;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  /* Set the priority of the client's TCP to the lowest */
  tcp_setprio(newpcb, TCP_PRIO_MIN);

  /* Dynamically assigned TCP_ECHOSERVER_STRUCT */
  es = (struct tcp_echoserver_struct *)mem_malloc(sizeof(struct tcp_echoserver_struct));
  // Only involve dynamic allocation, it is necessary to judge whether the return address is correct.
    if (es != NULL)
  {
    // The status of the current Server is completed for customer access
    es->state = ES_ACCEPTED;
    // PCB points to the customer's TCP control block
    es->pcb = newpcb;
    // Researchers, assignment is 0
    es->retries = 0;
    // Data receiving / send buffer points to empty
    es->p = NULL;

    /*
    	     There are a lot of members in each PCB control block. There is a member to store parameters related to the current TCP.
    	     TCP_ECHOSERVER_STRUCT with our entire program, as a parameter is incorporated into our PCB control block
    */
    tcp_arg(newpcb, es);

    /*
    	     Initialize the received callback function TCP_ECHOSERVER_RECV
    */
    tcp_recv(newpcb, tcp_echoserver_recv);

    /* Tune function after initialization errors */
    tcp_err(newpcb, tcp_echoserver_error);

    /* Initialize the callback function of the polling, but the intercal value is 0, that is, the polling function is not enabled */
    tcp_poll(newpcb, tcp_echoserver_poll, 0);

    ret_err = ERR_OK;
  }// Create TCP_ECHOSERVER_STRUCT failed
  else
  {
    /* Turn off TCP connection */
    tcp_echoserver_connection_close(newpcb, es);
    /* Return to Memory Allocation Failure */
    ret_err = ERR_MEM;
  }
  return ret_err;
}

/**
  * @brief  This function is the implementation for tcp_recv LwIP callback
     * @Param arg: This is TCP_ECHOSERVER_STRUCT
     * @Param TPCB: Currently use TCP control blocks
     * @Param PBUF: Points to the buffer that receives the data, this PBUF is automatically assigned after receiving the data inside the LWIP.
     * @Param Err: Error message
  * @retval err_t: error code
  */
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct tcp_echoserver_struct *es;
  err_t ret_err;

  LWIP_ASSERT("arg != NULL",arg != NULL);
  // Get the current TCP_ECHOSERVER_STRUCT
  es = (struct tcp_echoserver_struct *)arg;

  /* The received data is empty */
  if (p == NULL)
  {
    /* Change state is off */
    es->state = ES_CLOSING;
    // Judgment Do we also need to send data
    if(es->p == NULL)
    {
       /* Do not send, directly turn off TCP connection */
       tcp_echoserver_connection_close(tpcb, es);
    }
    else
    {
      /* Create a callback function after sending data */
      tcp_sent(tpcb, tcp_echoserver_sent);

      /* Trigger send, real send to NIC */
      tcp_echoserver_send(tpcb, es);
    }
    ret_err = ERR_OK;
  }
  /* Received data, but generated error */
  else if(err != ERR_OK)
  {
    /* Release the received buffer */
    if (p != NULL)
    {
      es->p = NULL;
      pbuf_free(p);
    }
    ret_err = err;
  }
  else if(es->state == ES_ACCEPTED)
  {
    /* Change status is received */
    es->state = ES_RECEIVED;

    /* Use the address of the received PBUF to access our local structure */
    es->p = p;

    /* Answer ------ Our function is echo, echo */
    tcp_sent(tpcb, tcp_echoserver_sent);

    /* Send data directly to NIC */
    tcp_echoserver_send(tpcb, es);

    ret_err = ERR_OK;
  }
  else if (es->state == ES_RECEIVED)
  {
    /* Judging that our transmission buffer is not issued, no */
    if(es->p == NULL)
    {
      // Store PBUF
      es->p = p;

      /* Transfer */
      tcp_echoserver_send(tpcb, es);
    }// There is a data need to be sent
    else
    {
      struct pbuf *ptr;

      /*
      	       Since the data is not sent, data connection is required.
      */
      ptr = es->p;// Get the address to be sent to
      pbuf_chain(ptr,p);// Tail connection
    }
    ret_err = ERR_OK;
  }// Is in a closed state
  else if(es->state == ES_CLOSING)
  {
    /*
    	     Release PBUF
    */
    tcp_recved(tpcb, p->tot_len);
    es->p = NULL;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  else
  {
    /* Release PBUF */
    tcp_recved(tpcb, p->tot_len);
    es->p = NULL;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

/**
  * @Brief When an error occurs, the LWIP calls this function.
     * @Param Arg: Point us TCP_ECHOSERVER_STRUCT
  * @param  err: not used
  * @retval None
  */
static void tcp_echoserver_error(void *arg, err_t err)
{
  struct tcp_echoserver_struct *es;

  LWIP_UNUSED_ARG(err);

  es = (struct tcp_echoserver_struct *)arg;
  if (es != NULL)
  {
    /* Release TCP_ECHOSERVER_STRUCT */
    mem_free(es);
  }
}

/**
  * @brief  This function implements the tcp_poll LwIP callback function
  * @param  arg: pointer on argument passed to callback
  * @param  tpcb: pointer on the tcp_pcb for the current tcp connection
  * @retval err_t: error code
  */
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct tcp_echoserver_struct *es;

  es = (struct tcp_echoserver_struct *)arg;
  if (es != NULL)
  {
    // There is a data need to be sent
    if (es->p != NULL)
    {

      tcp_sent(tpcb, tcp_echoserver_sent);
      /* there is a remaining pbuf (chain) , try to send data */
      tcp_echoserver_send(tpcb, es);
    }
    else
    {
      /* Determined whether the status is turned off */
      if(es->state == ES_CLOSING)
      {
        /* Turn off TCP connection */
        tcp_echoserver_connection_close(tpcb, es);
      }
    }
    ret_err = ERR_OK;
  }	//No TCP_ECHOSERVER_STRUCT
  else
  {
    /* Terminate TCP tasks */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

/**
     * @Brief When the send is complete, call this callback function
  * @param  None
  * @retval None
  */
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct tcp_echoserver_struct *es;

  LWIP_UNUSED_ARG(len);

  es = (struct tcp_echoserver_struct *)arg;
  es->retries = 0;

  if(es->p != NULL)
  {
    /* There is also data needs to continue to send */
    tcp_sent(tpcb, tcp_echoserver_sent);
    tcp_echoserver_send(tpcb, es);
  }
  else
  {
    /* Need to close, we close */
    if(es->state == ES_CLOSING)
      tcp_echoserver_connection_close(tpcb, es);
  }
  return ERR_OK;
}

/**
     * @brief sends data to the NIC
  * @param  tpcb: pointer on the tcp_pcb connection
  * @param  es: pointer on echo_state structure
  * @retval None
  */
static void tcp_echoserver_send(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
{
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;
 // Do three judgments
    /*
    	     1, TCP task has no error
    	     2, the data to be sent
    	     3, the data to be sent, the length is less than the TCP transmission length
    	     3.1, TCP sends BUF, there are restrictions, not infinite, we want to send the length, can not be greater than it, if it is greater than it will be wrong
    */
  while ((wr_err == ERR_OK) &&
         (es->p != NULL) &&
         (es->p->len <= tcp_sndbuf(tpcb)))
  {

    /* Get the PBUF pointer to be sent */
    ptr = es->p;

    /* Call WRITE to send, send it to the NIC */
    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);

    if (wr_err == ERR_OK)
    {
      u16_t plen;
      u8_t freed;
	  // Get the remaining length
      plen = ptr->len;

      /* If there are other in this PBUF list, then send */
      es->p = ptr->next;

      if(es->p != NULL)
      {
        /* Refresh count value */
        pbuf_ref(es->p);
      }

     /*  */
      do
      {
        /* Release PBUF */
        freed = pbuf_free(ptr);
      }
      while(freed == 0);
     /* Receive */
     tcp_recved(tpcb, plen);
   }
   else if(wr_err == ERR_MEM)
   {
      /* Reissue */
     es->p = ptr;
   }
   else
   {
     /* other problem ?? */
   }
  }
}

/**
  * @brief  This functions closes the tcp connection
  * @param  tcp_pcb: pointer on the tcp connection
  * @param  es: pointer on echo_state structure
  * @retval None
  */
static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
{

  /* Transplanted all callback functions */
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);

  /* Release TCP_ECHOSERVER_STRUCT Memory Space */
  if (es != NULL)
  {
    mem_free(es);
  }

  /* Close the TCP connection, here the PCB control block is released */
  tcp_close(tpcb);
}

#endif /* LWIP_TCP */
