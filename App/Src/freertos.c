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
#include "string.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "udp_server.h"
#include "can_driver.h"
typedef StaticQueue_t osStaticMessageQDef_t;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// normaly there will be more task specific struct but since we have no specyfiks of the process we are controlling
// we are just copeing the buffers
typedef struct
{
	uint8_t payload[UDP_MAX_MSG_SIZE];
	uint16_t length;
}App_udp_message_type;


typedef struct
{
	uint8_t Data_Buf[8];
	uint16_t length;
}App_can_message_type;

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
  .stack_size = 2048,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* Definitions for AppUdpPacketQueue */
osMessageQueueId_t AppUdpPacketQueueHandle;
uint8_t AppUdpPacketQueueBuffer[ APP_UDP_QUEUE_SIZE * sizeof(App_udp_message_type) ];
osStaticMessageQDef_t AppUdpPacketQueueControlBlock;
const osMessageQueueAttr_t AppUdpPacketQueueAttributes = {
  .name = "AppUdpPacketQueue",
  .cb_mem = &AppUdpPacketQueueControlBlock,
  .cb_size = sizeof(AppUdpPacketQueueControlBlock),
  .mq_mem = &AppUdpPacketQueueBuffer,
  .mq_size = sizeof(AppUdpPacketQueueBuffer)
};

/* Definitions for AppCANPacketQueue */
osMessageQueueId_t AppCANPacketQueueHandle;
uint8_t AppCANPacketQueueBuffer[ APP_CAN_QUEUE_SIZE * sizeof(App_can_message_type) ];
osStaticMessageQDef_t AppCANPacketQueueControlBlock;
const osMessageQueueAttr_t AppCANPacketQueueAttributes = {
  .name = "AppCANPacketQueue",
  .cb_mem = &AppCANPacketQueueControlBlock,
  .cb_size = sizeof(AppCANPacketQueueControlBlock),
  .mq_mem = &AppCANPacketQueueBuffer,
  .mq_size = sizeof(AppCANPacketQueueBuffer)
};



osThreadId_t CANTaskHandle;
const osThreadAttr_t CANTask_attributes = {
  .name = "CANTask",
  .stack_size = 2048,
  .priority = (osPriority_t) osPriorityNormal1,
};

/* Definitions for CANPacketQueue */
osMessageQueueId_t CANRxPacketQueueHandle;
uint8_t CANRxPacketQueueBuffer[ CAN_RX_PACKET_QUEUE_SIZE * sizeof( CAN_Message_type ) ];
osStaticMessageQDef_t CANRxPacketQueueControlBlock;
const osMessageQueueAttr_t CANRxPacketQueueAttributes = {
  .name = "CANPacketQueue",
  .cb_mem = &CANRxPacketQueueControlBlock,
  .cb_size = sizeof(CANRxPacketQueueControlBlock),
  .mq_mem = &CANRxPacketQueueBuffer,
  .mq_size = sizeof(CANRxPacketQueueBuffer)
};

osMessageQueueId_t CANTxPacketQueueHandle;
uint8_t CANTxPacketQueueBuffer[ CAN_RX_PACKET_QUEUE_SIZE  * sizeof( CAN_Message_type ) ];
osStaticMessageQDef_t CANTxPacketQueueControlBlock;
const osMessageQueueAttr_t CANTxPacketQueueAttributes = {
  .name = "CANTxPacketQueue",
  .cb_mem = &CANTxPacketQueueControlBlock,
  .cb_size = sizeof(CANTxPacketQueueControlBlock),
  .mq_mem = &CANTxPacketQueueBuffer,
  .mq_size = sizeof(CANTxPacketQueueBuffer)
};

osThreadId_t UDPTaskHandle;
const osThreadAttr_t UDPTask_attributes = {
  .name = "UDPTask",
  .stack_size = 2048,
  .priority = (osPriority_t) osPriorityNormal2,
};

/* Definitions for UdpPacketQueue */
osMessageQueueId_t UdpRxPacketQueueHandle;
uint8_t UdpRxPacketQueueBuffer[ UDP_RX_QUEUE_SIZE * sizeof(udp_message_type) ];
osStaticMessageQDef_t UdpRxPacketQueueControlBlock;
const osMessageQueueAttr_t UdpRxPacketQueueAttributes = {
  .name = "UdpRxPacketQueue",
  .cb_mem = &UdpRxPacketQueueControlBlock,
  .cb_size = sizeof(UdpRxPacketQueueControlBlock),
  .mq_mem = &UdpRxPacketQueueBuffer,
  .mq_size = sizeof(UdpRxPacketQueueBuffer)
};

osMessageQueueId_t UdpTxPacketQueueHandle;
uint8_t UdpTxPacketQueueBuffer[ UDP_TX_QUEUE_SIZE * sizeof(udp_message_type) ];
osStaticMessageQDef_t UdpTxPacketQueueControlBlock;
const osMessageQueueAttr_t UdpTxPacketQueueAttributes = {
  .name = "UdpTxPacketQueue",
  .cb_mem = &UdpTxPacketQueueControlBlock,
  .cb_size = sizeof(UdpTxPacketQueueControlBlock),
  .mq_mem = &UdpTxPacketQueueBuffer,
  .mq_size = sizeof(UdpTxPacketQueueBuffer)
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
  UdpRxPacketQueueHandle = osMessageQueueNew (UDP_RX_QUEUE_SIZE, sizeof(udp_message_type), &UdpRxPacketQueueAttributes);
  UdpTxPacketQueueHandle = osMessageQueueNew (UDP_TX_QUEUE_SIZE, sizeof(udp_message_type), &UdpTxPacketQueueAttributes);
  /* creation of CANPacketQueue */
  CANRxPacketQueueHandle = osMessageQueueNew (CAN_RX_PACKET_QUEUE_SIZE , sizeof(CAN_Message_type), &CANRxPacketQueueAttributes);
  CANTxPacketQueueHandle = osMessageQueueNew (CAN_TX_PACKET_QUEUE_SIZE , sizeof(CAN_Message_type), &CANTxPacketQueueAttributes);

  AppUdpPacketQueueHandle = osMessageQueueNew ( APP_UDP_QUEUE_SIZE , sizeof(App_udp_message_type), &AppUdpPacketQueueAttributes);
  AppCANPacketQueueHandle = osMessageQueueNew ( APP_UDP_QUEUE_SIZE , sizeof(App_can_message_type), &AppCANPacketQueueAttributes);
  /* USER CODE END RTOS_QUEUES */

  #define APP_CAN_QUEUE_SIZE 8
  /* Create the thread(s) */
  /* creation of defaultTask */


  /* USER CODE BEGIN RTOS_THREADS */
  	UDPTaskHandle = osThreadNew(UDPTask, NULL, &UDPTask_attributes);
  	CANTaskHandle = osThreadNew(CANTask, NULL, &CANTask_attributes);
  	AppTaskHandle = osThreadNew(AppTask, NULL, &AppTask_attributes);


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
	   App_udp_message_type udp_msg;
	   App_can_message_type can_msg;


	   	if(osOK == osMessageQueueGet(AppUdpPacketQueueHandle,&udp_msg,0,0))
	   	{
	   		//do smth
	   	}

	   	if(osOK == osMessageQueueGet(AppCANPacketQueueHandle,&can_msg,0,0))
	   	{
	   		//do smth
	   	}

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
		can_handler();
		osDelay(20);
	}
}

void UDPTask(void *argument)
{

	for(;;)
	{
		udp_handler();
		osDelay(5);

	}
}


/* USER CODE END Application */

