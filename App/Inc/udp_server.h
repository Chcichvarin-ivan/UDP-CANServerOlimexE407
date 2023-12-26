/*
 * udp_server.h
 *
 *  Created on: Dec 20, 2023
 *      Author: Ivan Chichvarin
 */
#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__
#define UDP_MAX_MSG_SIZE		512
#define UDP_RX_QUEUE_SIZE			16
#define UDP_TX_QUEUE_SIZE			16
#include "lwip.h"
#include "lwip/arch.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "stdbool.h"

typedef struct
{
	u32_t ip_addr;
	u16_t port;
	u16_t length;
	u8_t udp_recvbuf[UDP_MAX_MSG_SIZE];
}udp_message_type;

bool udp_server_push_packet(const ip_addr_t *,u16_t,struct pbuf *);
void udp_server_send(udp_message_type);
void udp_handler(void);

#endif /* __CAN_H__ */
