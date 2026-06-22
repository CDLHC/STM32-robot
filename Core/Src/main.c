/* USER CODE BEGIN Header */
  /**
    ******************************************************************************
    * @file           : main.c
    * @brief          : Main program body
    ******************************************************************************
    * @attention
    *
    * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"
#include "cmsis_os.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
  #include "OLED.h"
  #include "LED.h"
  #include "PWM.h"
  #include "BOO.h"
  #include "Input.h"
  #include "Servo.h"
  #include "Serial.h"
  #include "Input.h"
  #include <string.h>
  #include <stdlib.h>
  #include <stdio.h>
  #include <math.h>
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

/* USER CODE BEGIN PV */

  extern const unsigned char BiLi[];
  extern const unsigned char DOUYIN[];
  extern const unsigned char LiC[];

  volatile float Angle1 = 90.0f;
  volatile float Angle2 = 90.0f;
  volatile float Angle3 = 90.0f;
  volatile float Angle4 = 90.0f;
  volatile float Angle5 = 0.0f;
  volatile float Angle6 = 180.0f;
  volatile float Angle7 = 0.0f;
  volatile float Angle8 = 180.0f;
  const float MinServoStep = 0.5f;
  const float MaxServoStep = 8.0f;
  const float FastThreshold = 20.0f;
  const float SlowThreshold = 2.0f;

  volatile GaitMode_t gaitMode = GAIT_IDLE;
  volatile uint8_t  gaitStep = 0;
  volatile uint16_t gaitDelayCnt = 0;

  const uint16_t GAIT_DELAY_FORWARD   = 8;
  const uint16_t GAIT_DELAY_BACKWARD  = 8;
  const uint16_t GAIT_DELAY_TURNLEFT  = 10;
  const uint16_t GAIT_DELAY_TURNRIGHT = 10;
  const uint16_t GAIT_DELAY_ROTATECW  = 10;
  const uint16_t GAIT_DELAY_ROTATECCW = 10;
  const uint16_t GAIT_DELAY_SIT       = 12;

  uint8_t oledRefreshCnt = 0;
  const uint8_t OLED_REFRESH_INTERVAL = 8;

  volatile uint8_t LED_State = 0;
  volatile uint16_t LED_DelayCnt = 0;
  const uint16_t LED_DELAY_TIME = 12;

  volatile uint8_t Huadong_State = 0;
  volatile uint8_t Huadong_Step = 0;
  const uint8_t MAX_STEP = 15;
  const uint8_t STEP_INTERVAL = 1;
  volatile uint8_t step_delay = 0;
  volatile uint8_t huadong_release_flag = 0;

  uint16_t booOffCnt = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
  void SetStandPosture(void) {
       Angle1 = 135.0f; Angle2 = 45.0f;
       Angle3 = 45.0f;  Angle4 = 135.0f;
       Angle5 = 90.0f;  Angle6 = 90.0f;
       Angle7 = 90.0f;  Angle8 = 90.0f;
   }
  void ResetAll(void)
  {
	  SetStandPosture();

      gaitMode = GAIT_IDLE;
      gaitStep = 0;
      gaitDelayCnt = 0;
      OLED_Clear();
      OLED_ShowImage(0, 0, 128, 64, happy);
      OLED_Update();
  }

  void Huadong_Smooth(void)
   {
       OLED_Clear();
       OLED_ShowImage(0, 0, 128, 64, sad);
       OLED_Update();

       // 肩膀保持站立姿态
       Angle1 = 135.0f;
       Angle2 = 45.0f;
       Angle3 = 45.0f;
       Angle4 = 135.0f;
       Servo_SetAngle1(Angle1);
       Servo_SetAngle2(Angle2);
       Servo_SetAngle3(Angle3);
       Servo_SetAngle4(Angle4);

       step_delay++;
       if (step_delay < STEP_INTERVAL) return;
       step_delay = 0;

       switch(Huadong_State)
       {
           case 1:
               Huadong_Step++;
               if (Huadong_Step >= MAX_STEP)
               {
                   Huadong_Step = MAX_STEP;
                   Huadong_State = 2;
               }
               break;
           case 2:
               Huadong_Step--;
               if (Huadong_Step <= 0)
               {
                   Huadong_Step = 0;
                   Huadong_State = 1;
               }
               break;
           default:
               return;
       }

       // 四膝从90向中心收拢再展开
       Angle5 = 90 - (Huadong_Step * 2);   // 90 → 60 → 90
       Angle7 = 90 - (Huadong_Step * 2);
       Angle6 = 90 + (Huadong_Step * 2);   // 90 → 120 → 90
       Angle8 = 90 + (Huadong_Step * 2);

       Servo_SetAngle5(Angle5); Servo_SetAngle6(Angle6);
       Servo_SetAngle7(Angle7); Servo_SetAngle8(Angle8);
   }


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART3_UART_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
    Servo_Init();
    Input_Init();
    BOO_Init();
    OLED_Init();
    LED_Init();
    LED_StartBlink();
    ResetAll();
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
