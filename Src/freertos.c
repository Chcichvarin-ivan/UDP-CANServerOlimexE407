/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "udp_server.h"
#include "string.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
typedef StaticQueue_t osStaticMessageQDef_t;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t AppTaskHandle;
const osThreadAttr_t AppTask_attributes = {
  .name = "AppTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
osThreadId_t CANTaskHandle;
const osThreadAttr_t CANTask_attributes = {
  .name = "CANTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for CANPacketQueue */
osMessageQueueId_t CANPacketQueueHandle;
uint8_t CANPacketQueueBuffer[ 16 * sizeof( uint16_t ) ];
osStaticMessageQDef_t CANPacketQueueControlBlock;
const osMessageQueueAttr_t CANPacketQueue_attributes = {
  .name = "CANPacketQueue",
  .cb_mem = &CANPacketQueueControlBlock,
  .cb_size = sizeof(CANPacketQueueControlBlock),
  .mq_mem = &CANPacketQueueBuffer,
  .mq_size = sizeof(CANPacketQueueBuffer)
};

osThreadId_t UDPTaskHandle;
const osThreadAttr_t UDPTask_attributes = {
  .name = "UDPTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for UdpPacketQueue */
osMessageQueueId_t UdpPacketQueueHandle;
uint8_t UdpPacketQueueBuffer[ UDP_QUEUE_SIZE * sizeof(udp_message_type) ];
osStaticMessageQDef_t UdpPacketQueueControlBlock;
const osMessageQueueAttr_t UdpPacketQueue_attributes = {
  .name = "UdpPacketQueue",
  .cb_mem = &UdpPacketQueueControlBlock,
  .cb_size = sizeof(UdpPacketQueueControlBlock),
  .mq_mem = &UdpPacketQueueBuffer,
  .mq_size = sizeof(UdpPacketQueueBuffer)
};

void AppTask(void *argument);
void CANTask(void *argument);
void UDPTask(void *argument);
/* USER CODE END FunctionPrototypes */



extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */

  /* Create the queue(s) */
  /* creation of UdpPacketQueue */
  UdpPacketQueueHandle = osMessageQueueNew (UDP_QUEUE_SIZE, sizeof(udp_message_type), &UdpPacketQueue_attributes);

  /* creation of CANPacketQueue */
  CANPacketQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &CANPacketQueue_attributes);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */


  /* USER CODE BEGIN RTOS_THREADS */
	AppTaskHandle = osThreadNew(AppTask, NULL, &AppTask_attributes);
	CANTaskHandle = osThreadNew(CANTask, NULL, &CANTask_attributes);
	UDPTaskHandle = osThreadNew(UDPTask, NULL, &UDPTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
void sign_of_life(void)
{
	static int ms_count;
	if (ms_count > 100)
	{
		ms_count = 0;
	    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	}else
	{
	       ms_count++;
	}
}
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
extern void udp_server_init(void);
/* USER CODE END Header_StartDefaultTask */
void AppTask(void *argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  udp_server_init();

   /* Infinite loop */
   for(;;)
   {
	   sign_of_life();
	   osDelay(10);
   }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void CANTask(void *argument)
{
	for(;;)
	{
		osDelay(20);
	}
}

void UDPTask(void *argument)
{

	for(;;)
	{
		udp_message_type in_msg;
		osMessageQueueGet(UdpPacketQueueHandle,&in_msg,0,osWaitForever);
		udp_server_send(in_msg);
		osDelay(5);
	}
}

bool udp_server_push_packet(const ip_addr_t *in_addr,u16_t in_port,struct pbuf *in_buf)
{
	bool ret_val = false;
	osStatus_t is_enqueued;
	udp_message_type in_msg;
	in_msg.ip_addr = in_addr->addr;
	in_msg.port = in_port;
	memset(in_msg.udp_recvbuf,0,UDP_MAX_MSG_SIZE);
	memcpy(in_msg.udp_recvbuf,in_buf->payload,in_buf->len);
	is_enqueued  = osMessageQueuePut(UdpPacketQueueHandle,&in_msg,0,osWaitForever);
	if(is_enqueued == osOK)
	{
		ret_val = true;
	}else
	{
		ret_val = false;
	}
	return ret_val;
}
/* USER CODE END Application */

