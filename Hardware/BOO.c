  #include "main.h"

  void BOO_Init(void)
  {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  }

  void BOO_ON(void)
  {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
  }

  void BOO_OFF(void)
  {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
  }

  void HW_ON(void)
  {
      BOO_ON();
  }

  void HW_OFF(void)
  {
      BOO_OFF();
  }
	
	
