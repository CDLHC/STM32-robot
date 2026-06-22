#include "main.h"
extern volatile uint8_t LED_State;
extern volatile uint16_t LED_DelayCnt;
void LED_Init(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12 | GPIO_PIN_15, GPIO_PIN_SET);
}

void LED1_ON(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
}

void LED1_OFF(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
}



void LED2_ON(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15, GPIO_PIN_RESET);
}

void LED2_OFF(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15, GPIO_PIN_SET);
}


void LED_StartBlink(void)
{
    LED_State = 1;  
    LED_DelayCnt = 0;
}


void LED_StopBlink(void)
{
    LED_State = 0;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12 | GPIO_PIN_15, GPIO_PIN_SET);
}
