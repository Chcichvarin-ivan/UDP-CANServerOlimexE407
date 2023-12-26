/*
 * can_driver.c
 *
 *  Created on: Dec 25, 2023
 *      Author: Ivan Chichvarin
 */


#include "can_driver.h"

CAN_TxHeaderTypeDef TxHeader = {
		  .StdId = 0x7FF,
		  .ExtId = 0,
		  .RTR = CAN_RTR_DATA,
		  .IDE = CAN_ID_STD,
		  .DLC = 1,
		  .TransmitGlobalTime = 0,
};

uint32_t TxMailbox = 0;
CAN_RxHeaderTypeDef RxHeader;
extern CAN_HandleTypeDef hcan2;
extern osMessageQueueId_t CANTxPacketQueueHandle;
extern osMessageQueueId_t CANRxPacketQueueHandle;

void can_handler(void)
{

	CAN_Message_type can_tx_buf;
	CAN_Message_type can_rx_buf;
	osStatus_t status_tx;
	osStatus_t status_rx;

	//extern osEventFlagsId_t Control_Unit_EventsHandle;

	status_tx = osMessageQueueGet(CANTxPacketQueueHandle, &can_tx_buf, NULL, 0);
	if(status_tx == osOK)
	{
		while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) == 0);
		TxHeader.StdId = can_tx_buf.ID;
		TxHeader.DLC = can_tx_buf.DLC;
	    HAL_CAN_AddTxMessage(&hcan2, &TxHeader, can_tx_buf.Data_Buf, &TxMailbox);
	}


	status_rx = osMessageQueueGet(CANRxPacketQueueHandle, &can_rx_buf, NULL, 0);
	if(status_rx == osOK)
		{
			switch (can_rx_buf.ID)
			{
				case(ERROR_MESSAGE_ID):
					//process error situation
				break;
				//Process all id's

				default:
				break;
			}
		}
}


void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	CAN_Message_type can_rx_buf;

    if(HAL_CAN_GetRxMessage(&hcan2, CAN_RX_FIFO0, &RxHeader, can_rx_buf.Data_Buf) == HAL_OK)
    {
    	can_rx_buf.ID = RxHeader.StdId;
    	osMessageQueuePut(CANRxPacketQueueHandle, &can_rx_buf, 0U, 0U);
    }
}





void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	//TO DO: обработчик ошибок CAN
}
