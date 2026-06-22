 #include "main.h"
 #include "tim.h"
  void PWM_Init_TIM2(void)
  {
      HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
      HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
      HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
      HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
  }

  void PWM_SetCompare1(uint16_t Compare)
  {
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, Compare);
  }
  void PWM_SetCompare2(uint16_t Compare)
  {
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, Compare);
  }
  void PWM_SetCompare3(uint16_t Compare)
  {
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, Compare);
  }
  void PWM_SetCompare4(uint16_t Compare)
  {
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, Compare);
  }



  void PWM_Init_TIM3(void)
  {
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
      HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  }

  void PWM_SetCompare1_TIM3(uint16_t Compare)
  {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, Compare);
  }
  void PWM_SetCompare2_TIM3(uint16_t Compare)
  {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, Compare);
  }
  void PWM_SetCompare3_TIM3(uint16_t Compare)
  {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, Compare);
  }
  void PWM_SetCompare4_TIM3(uint16_t Compare)
  {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, Compare);
  }



  void PWM_Init_TIM4(void)
  {
      // TIM4 由 WS2812B 驱动直接寄存器操作，此处无需启动
  }
