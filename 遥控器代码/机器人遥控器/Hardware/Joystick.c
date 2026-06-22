#include "Joystick.h"
#include "stm32f10x.h"

/*
引脚映射：
  J1_X (左摇杆 X):  PA0 - ADC1_CH0
  J1_Y (左摇杆 Y):  PA1 - ADC1_CH1
  J2_X (右摇杆 X):  PA2 - ADC1_CH2
  J2_Y (右摇杆 Y):  PB0 - ADC1_CH8
  J1_SW (左摇杆按键):  PB1 - GPIO 上拉输入 (0=按下)
  J2_SW (右摇杆按键): PB11 - GPIO 上拉输入 (0=按下)
*/

void Joystick_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    ADC_InitTypeDef ADC_InitStruct;

    /* 使能时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_ADC1, ENABLE);

    /* PA0, PA1, PA2 设为模拟输入（摇杆轴） */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PB0 设为模拟输入（J2_Y） */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PB1, PB11 上拉输入（摇杆按键） */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* ADC1: 独立模式，单次转换，不连续，无外部触发 */
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStruct);

    /* ADC 时钟 = 72MHz / 6 = 12MHz (最大 14MHz) */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* 使能 ADC1 */
    ADC_Cmd(ADC1, ENABLE);

    /* 校准 */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

/* 读取单个ADC通道 */
static uint16_t ADC_ReadChannel(uint8_t adc_channel)
{
    /* 设置通道，采样时间 55.5 周期 */
    ADC_RegularChannelConfig(ADC1, adc_channel, 1, ADC_SampleTime_55Cycles5);

    /* 开始转换 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    /* 等待转换完成 */
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

    return ADC_GetConversionValue(ADC1);
}

void Joystick_Read(uint16_t *j1_x, uint16_t *j1_y,
                   uint16_t *j2_x, uint16_t *j2_y)
{
    *j1_x = ADC_ReadChannel(ADC_Channel_0);   /* PA0 */
    *j1_y = ADC_ReadChannel(ADC_Channel_1);   /* PA1 */
    *j2_x = ADC_ReadChannel(ADC_Channel_2);   /* PA2 */
    *j2_y = ADC_ReadChannel(ADC_Channel_8);   /* PB0 */
}

uint8_t Joystick_SW1_Pressed(void)
{
    return (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == Bit_RESET) ? 1 : 0;
}

uint8_t Joystick_SW2_Pressed(void)
{
    return (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == Bit_RESET) ? 1 : 0;
}
