#include "main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "queue.h"
uint8_t Voice_RxData;

uint8_t Serial_RxData;     
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

extern QueueHandle_t uartRxQueue;
void Serial_Init(void)
{
    HAL_UART_Receive_IT(&huart1, &Serial_RxData, 1);
}

void Voice_Init(void)
{
    HAL_UART_Receive_IT(&huart3, &Voice_RxData, 1);
}


void Serial_SendByte(uint8_t Byte)
{
    HAL_UART_Transmit(&huart1, &Byte, 1, HAL_MAX_DELAY);
}

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t data;

    if (huart->Instance == USART1)
    {
        data = Serial_RxData;
        xQueueSendFromISR(uartRxQueue, &data, &xHigherPriorityTaskWoken);
        HAL_UART_Receive_IT(&huart1, &Serial_RxData, 1);
    }
    else if (huart->Instance == USART3)
    {
        data = Voice_RxData;
        xQueueSendFromISR(uartRxQueue, &data, &xHigherPriorityTaskWoken);
        HAL_UART_Receive_IT(&huart3, &Voice_RxData, 1);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
