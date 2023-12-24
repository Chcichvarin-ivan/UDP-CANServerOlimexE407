/*
 * udp_server.h
 *
 *  Created on: Dec 20, 2023
 *      Author: Ivan Chichvarin
 */
#ifndef __UDP_SERVER_H__
#define __UDP_SERVER_H__
#define UDP_MAX_MSG_SIZE		1024
#define UDP_QUEUE_SIZE			32

#include "lwip/udp.h"
#include "stdbool.h"

typedef struct
{
	u32_t ip_addr;
	u16_t port;
	u8_t udp_recvbuf[UDP_MAX_MSG_SIZE];
}udp_message_type;

bool udp_server_push_packet(const ip_addr_t *,u16_t,struct pbuf *);
void udp_server_send(udp_message_type);

#endif /* __CAN_H__ */
