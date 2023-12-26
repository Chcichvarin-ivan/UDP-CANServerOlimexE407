/*
 * can_driver.h
 *
 *  Created on: Dec 25, 2023
 *      Author: Ivan Chichvarin
 */

#ifndef CAN_DRIVER_H_
#define CAN_DRIVER_H_

#include "main.h"

#define CAN_RX_PACKET_QUEUE_SIZE 8
#define CAN_TX_PACKET_QUEUE_SIZE 8

#define ERROR_MESSAGE_ID							0b101

typedef struct {
	  uint8_t Data_Buf[8];
	  uint32_t ID;
	  uint8_t  DLC;					//number of data bytes in CAN frame
	} CAN_Message_type;


void can_handler(void);
#endif /* CAN_DRIVER_H_ */
