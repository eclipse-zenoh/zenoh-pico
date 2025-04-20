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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stream_buffer.h"
#include "usart.h"
#include "zenoh-pico/config.h"
#include "zenoh.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SYS_READY					(1 << 0)
#define COM_READY					(1 << 1)
#define SERIAL_BUFFER_SIZE			32
#define SERIAL_STREAM_BUFFER_SIZE	1024
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static uint8_t rxBuffer[SERIAL_BUFFER_SIZE];
static uint8_t txBuffer[SERIAL_BUFFER_SIZE];
static volatile size_t rxBufferHead;
static volatile uint8_t rxEnable;
static uint8_t ucSerialRxStreamBufferStorage[SERIAL_STREAM_BUFFER_SIZE + 1];
static StaticStreamBuffer_t xSerialRxStreamBufferStruct;
StreamBufferHandle_t xSerialRxStreamBuffer;
static uint8_t ucSerialTxStreamBufferStorage[SERIAL_STREAM_BUFFER_SIZE + 1];
static StaticStreamBuffer_t xSerialTxStreamBufferStruct;
StreamBufferHandle_t xSerialTxStreamBuffer;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
uint32_t defaultTaskBuffer[ 2048 ];
osStaticThreadDef_t defaultTaskControlBlock;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .cb_mem = &defaultTaskControlBlock,
  .cb_size = sizeof(defaultTaskControlBlock),
  .stack_mem = &defaultTaskBuffer[0],
  .stack_size = sizeof(defaultTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for serialPortTask */
osThreadId_t serialPortTaskHandle;
uint32_t serialPortTaskBuffer[ 256 ];
osStaticThreadDef_t serialPortTaskControlBlock;
const osThreadAttr_t serialPortTask_attributes = {
  .name = "serialPortTask",
  .cb_mem = &serialPortTaskControlBlock,
  .cb_size = sizeof(serialPortTaskControlBlock),
  .stack_mem = &serialPortTaskBuffer[0],
  .stack_size = sizeof(serialPortTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for bootEvents */
osEventFlagsId_t bootEventsHandle;
const osEventFlagsAttr_t bootEvents_attributes = {
  .name = "bootEvents"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartSerialPortTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= 1;
	DWT->CYCCNT = 0;
}

__weak unsigned long getRunTimeCounterValue(void)
{
return DWT->CYCCNT;
}
/* USER CODE END 1 */

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
  xSerialRxStreamBuffer = xStreamBufferCreateStatic(
		  SERIAL_STREAM_BUFFER_SIZE, 1,
		  ucSerialRxStreamBufferStorage,
		  &xSerialRxStreamBufferStruct);
  xSerialTxStreamBuffer = xStreamBufferCreateStatic(
		  SERIAL_STREAM_BUFFER_SIZE, 1,
		  ucSerialTxStreamBufferStorage,
		  &xSerialTxStreamBufferStruct);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of serialPortTask */
  serialPortTaskHandle = osThreadNew(StartSerialPortTask, NULL, &serialPortTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of bootEvents */
  bootEventsHandle = osEventFlagsNew(&bootEvents_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  GPIO_PinState st = GPIO_PIN_RESET;

  // Startup application
  osEventFlagsSet(bootEventsHandle, SYS_READY);

  // Wait for the tasks to become ready
  osEventFlagsWait(bootEventsHandle, COM_READY, osFlagsWaitAll | osFlagsNoClear, osWaitForever);

  // Initialize Zenoh services
  rxEnable = 1;
  if (Z_Init() < 0) {
	Error_Handler();
  }

  /* Infinite loop */
  for(;;)
  {
	GPIO_PinState _st = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
	if (_st != st) {
		if (_st == GPIO_PIN_SET) {
			Z_TickCount(osKernelGetTickCount());
			Z_GetAsync();
		} else {
			Z_GetSync();
		}
	}
	st = _st;
	osDelay(10);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartSerialPortTask */
/**
* @brief Function implementing the serialPortTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSerialPortTask */
void StartSerialPortTask(void *argument)
{
  /* USER CODE BEGIN StartSerialPortTask */
  size_t received;

  // Start receiving data from UART to the circular buffer
  rxBufferHead = 0;
  rxEnable = 0;
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rxBuffer, sizeof(rxBuffer));

  osEventFlagsWait(bootEventsHandle, SYS_READY, osFlagsNoClear, osWaitForever);
  osEventFlagsSet(bootEventsHandle, COM_READY);

  /* Infinite loop */
  for(;;)
  {
	// Transmit buffer data over the UART interface
	received = xStreamBufferReceive(
			xSerialTxStreamBuffer,
			txBuffer, sizeof(txBuffer),
			portMAX_DELAY);
	HAL_UART_Transmit_DMA(&huart2, txBuffer, received);
	ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
  }
  /* USER CODE END StartSerialPortTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (huart == Z_UART) {

		// This callback is called on the following events:
		// 	RxHalfCplt : from HAL_DMA_IRQHandler, Size == sizeof(rxBuffer) / 2
		//	RxCplt     : from HAL_DMA_IRQHandler, Size == sizeof(rxBuffer)
		//	Idle       : from HAL_UART_IRQHandler, rxBufferHead < Size < sizeof(rxBuffer)

		// Push the incoming data to the RX stream-buffer
		if (Size > rxBufferHead) {	// unprocessed data available
			Size -= rxBufferHead;
			if (rxEnable) {
				Size = xStreamBufferSendFromISR(xSerialRxStreamBuffer,
						rxBuffer + rxBufferHead, Size,
						&xHigherPriorityTaskWoken);
			}
			rxBufferHead += Size;
			rxBufferHead %= sizeof(rxBuffer);
		}

	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == Z_UART) {
		// Notify the send task that all data has been sent
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		vTaskNotifyGiveFromISR(serialPortTaskHandle, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart == Z_UART) {
		// Restart reception on error
		HAL_UART_Abort(&huart2);
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rxBuffer, sizeof(rxBuffer));
	}
}
/* USER CODE END Application */

