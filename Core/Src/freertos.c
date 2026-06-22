/* USER CODE BEGIN Header */
  /**
    ******************************************************************************
    * File Name          : freertos.c
    * Description        : Code for freertos applications
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
  #include "OLED.h"
  #include "LED.h"
  #include "BOO.h"
  #include "Servo.h"
  #include "Serial.h"
  #include "WS2812B.h"
  #include "NRF24L01.h"
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
/* USER CODE BEGIN Variables */

  //外部变量：定义在 main.c 里，这里用 extern 引用
  extern volatile float Angle1, Angle2, Angle3, Angle4;
  extern volatile float Angle5, Angle6, Angle7, Angle8;
  extern const float MinServoStep, MaxServoStep, FastThreshold, SlowThreshold;

  // 步态状态机
  extern volatile GaitMode_t gaitMode;
  extern volatile uint8_t  gaitStep;
  extern volatile uint16_t gaitDelayCnt;
  extern const uint16_t GAIT_DELAY_FORWARD, GAIT_DELAY_BACKWARD, GAIT_DELAY_TURNLEFT,
                        GAIT_DELAY_TURNRIGHT, GAIT_DELAY_ROTATECW, GAIT_DELAY_ROTATECCW,
                        GAIT_DELAY_SIT;

  // 传感器/滑动
  extern volatile uint8_t Huadong_State, Huadong_Step;
  extern volatile uint8_t step_delay, huadong_release_flag;
  extern const uint8_t MAX_STEP, STEP_INTERVAL;

// LED 模式（可由串口/蓝牙切换），开机默认关灯，StartLedTask 不抢 WS2812B
volatile LedMode_t ledMode = LED_MODE_OFF;

  // main.c 里的函数
  extern void Huadong_Smooth(void);
  extern void SetStandPosture(void);

  extern UART_HandleTypeDef huart1;
  extern UART_HandleTypeDef huart3;
  extern uint8_t Serial_RxData;
  extern uint8_t Voice_RxData;
  QueueHandle_t uartRxQueue;
  TaskHandle_t gaitTaskHandle;
  TimerHandle_t oledTimer;
  TimerHandle_t ledBlinkTimer;
  SemaphoreHandle_t angleMutex;   // 保护 Angle1~Angle8 多任务并发读写

/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
  void StartServoTask(void * argument);
  void StartSerialTask(void * argument);
  void StartGaitTask(void * argument);
  void StartSensorTask(void * argument);
  void StartLedTask(void * argument);
  void StartNRFTask(void * argument);
  void OledRefreshCallback(TimerHandle_t xTimer);
  void LedBlinkCallback(TimerHandle_t xTimer);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
    angleMutex = xSemaphoreCreateMutex();
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    oledTimer = xTimerCreate("OLED", pdMS_TO_TICKS(200), pdTRUE, NULL, OledRefreshCallback);
    ledBlinkTimer = xTimerCreate("LED", pdMS_TO_TICKS(300), pdTRUE, NULL, LedBlinkCallback);
    if (oledTimer) xTimerStart(oledTimer, 0);
    if (ledBlinkTimer) xTimerStart(ledBlinkTimer, 0);
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	uartRxQueue = xQueueCreate(128, sizeof(uint8_t));
	WS2812B_Init(8);
	Serial_Init();
	Voice_Init();
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
    xTaskCreate(StartServoTask,  "Servo",  128, NULL, osPriorityHigh,   NULL);
    xTaskCreate(StartSerialTask, "Serial", 256, NULL, osPriorityNormal, NULL);
    xTaskCreate(StartGaitTask,   "Gait",   512, NULL, osPriorityNormal, &gaitTaskHandle);
    xTaskCreate(StartSensorTask, "Sensor", 256, NULL, osPriorityNormal, NULL);
    xTaskCreate(StartLedTask,    "LED",    256, NULL, osPriorityLow,    NULL);
    xTaskCreate(StartNRFTask,    "NRF",    256, NULL, osPriorityNormal, NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  // 开机红色闪烁两次
  WS2812B_FlashRed(2, 500);
  // 闪完后灯带关闭，等待蓝牙指令（0x20 彩虹，0x21 关灯）
  for(;;) { osDelay(1000); }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* 自动演示：立正→前进→挥手→旋转→后退→蹲下 */
static void RunAutoDemo(void)
{
    ledMode = LED_MODE_RAINBOW;
    xSemaphoreTake(angleMutex, portMAX_DELAY); SetStandPosture(); xSemaphoreGive(angleMutex);
    vTaskDelay(pdMS_TO_TICKS(1000));

    gaitMode = GAIT_FORWARD; gaitStep = 1; gaitDelayCnt = 0;
    xTaskNotifyGive(gaitTaskHandle);
    vTaskDelay(pdMS_TO_TICKS(3000));

    gaitMode = GAIT_IDLE; xSemaphoreTake(angleMutex, portMAX_DELAY); SetStandPosture(); xSemaphoreGive(angleMutex);
    vTaskDelay(pdMS_TO_TICKS(500));

    for (int wave = 0; wave < 3; wave++) {
        for (float t = 0; t <= 1.0f; t += 0.1f) {
            xSemaphoreTake(angleMutex, portMAX_DELAY);
            Angle1 = 135.0f - 30.0f * t;
            Angle5 = 90.0f - 55.0f * t;
            xSemaphoreGive(angleMutex);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        for (float t = 0; t <= 1.0f; t += 0.1f) {
            xSemaphoreTake(angleMutex, portMAX_DELAY);
            Angle1 = 105.0f + 60.0f * t;
            Angle5 = 35.0f + 110.0f * t;
            xSemaphoreGive(angleMutex);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        for (float t = 0; t <= 1.0f; t += 0.1f) {
            xSemaphoreTake(angleMutex, portMAX_DELAY);
            Angle1 = 165.0f - 30.0f * t;
            Angle5 = 145.0f - 55.0f * t;
            xSemaphoreGive(angleMutex);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    for (int wave = 0; wave < 3; wave++) {
        for (float t = 0; t <= 1.0f; t += 0.1f) {
            xSemaphoreTake(angleMutex, portMAX_DELAY);
            Angle2 = 45.0f + 30.0f * t;
            Angle6 = 90.0f + 55.0f * t;
            xSemaphoreGive(angleMutex);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        for (float t = 0; t <= 1.0f; t += 0.1f) {
            xSemaphoreTake(angleMutex, portMAX_DELAY);
            Angle2 = 75.0f - 60.0f * t;
            Angle6 = 145.0f - 110.0f * t;
            xSemaphoreGive(angleMutex);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        for (float t = 0; t <= 1.0f; t += 0.1f) {
            xSemaphoreTake(angleMutex, portMAX_DELAY);
            Angle2 = 15.0f + 30.0f * t;
            Angle6 = 35.0f + 55.0f * t;
            xSemaphoreGive(angleMutex);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    xSemaphoreTake(angleMutex, portMAX_DELAY); SetStandPosture(); xSemaphoreGive(angleMutex);

    gaitMode = GAIT_ROTATE_CW; gaitStep = 1; gaitDelayCnt = 0;
    xTaskNotifyGive(gaitTaskHandle);
    vTaskDelay(pdMS_TO_TICKS(2000));

    gaitMode = GAIT_BACKWARD; gaitStep = 1; gaitDelayCnt = 0;
    xTaskNotifyGive(gaitTaskHandle);
    vTaskDelay(pdMS_TO_TICKS(3000));

    gaitMode = GAIT_SIT; gaitStep = 1; gaitDelayCnt = 0;
    xTaskNotifyGive(gaitTaskHandle);
    vTaskDelay(pdMS_TO_TICKS(2000));

    xSemaphoreTake(angleMutex, portMAX_DELAY); SetStandPosture(); xSemaphoreGive(angleMutex);
    ledMode = LED_MODE_OFF;
    gaitMode = GAIT_IDLE;
}

void StartServoTask(void * argument)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(20);

    for(;;)
    {
        // 上锁：确保 9 个角度值读取时的一致性（不被步态任务中途打断）
        xSemaphoreTake(angleMutex, portMAX_DELAY);
        Servo_SetAngle1(Angle1);
        Servo_SetAngle2(Angle2);
        Servo_SetAngle3(Angle3);
        Servo_SetAngle4(Angle4);
        Servo_SetAngle5(Angle5);
        Servo_SetAngle6(Angle6);
        Servo_SetAngle7(Angle7);
        Servo_SetAngle8(Angle8);
        xSemaphoreGive(angleMutex);

        vTaskDelayUntil(&xLastWakeTime, period);
    }
}


void StartSerialTask(void * argument)
{
    uint8_t cmd;

    while (xQueueReceive(uartRxQueue, &cmd, 0) == pdTRUE);
    for(;;)
    {
        if (xQueueReceive(uartRxQueue, &cmd, portMAX_DELAY) == pdTRUE)
        {
            do {
                switch (cmd) {
                    case 0x01: gaitMode = GAIT_FORWARD;    break;
                    case 0x06: gaitMode = GAIT_BACKWARD;   break;
                    case 0x02: gaitMode = GAIT_TURN_RIGHT; break;
                    case 0x03: gaitMode = GAIT_TURN_LEFT;  break;
                    case 0x04: gaitMode = GAIT_ROTATE_CW;  break;
                    case 0x05: gaitMode = GAIT_ROTATE_CCW; break;
                    case 0x10: gaitMode = GAIT_SIT;        break;
                    case 0x00: gaitMode = GAIT_IDLE;       break;
                    case 0x07: gaitMode = GAIT_IDLE; xSemaphoreTake(angleMutex, portMAX_DELAY); SetStandPosture(); xSemaphoreGive(angleMutex); break;
                    // LED 控制命令
                    case 0x09: ledMode = LED_MODE_RAINBOW;   break;
                    case 0x21: ledMode = LED_MODE_OFF;       break;
                    case 0x08: RunAutoDemo(); break;
                    default: break;
                }
                if (gaitMode != GAIT_IDLE || cmd == 0x07) {
                    gaitStep = 1;
                    gaitDelayCnt = 0;
                    xTaskNotifyGive(gaitTaskHandle);
                }
            } while (xQueueReceive(uartRxQueue, &cmd, 0) == pdTRUE);
        }

        vTaskDelay(pdMS_TO_TICKS(25));
    }
}


void StartGaitTask(void * argument)
{
    for(;;)
    {
        // 休眠等待指令
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (gaitMode != GAIT_IDLE)
        {
            // 上锁：步态写角度时禁止舵机读，避免读到半组数据
            xSemaphoreTake(angleMutex, portMAX_DELAY);
            // 前进步态
            if (gaitMode == GAIT_FORWARD)
        {
            gaitDelayCnt++;
            if (gaitDelayCnt >= GAIT_DELAY_FORWARD)
            {
                gaitDelayCnt = 0;
                if (gaitStep > 4) gaitStep = 1;
                else gaitStep++;
            }
            switch(gaitStep)
            {
                case 1:
                    Angle1=105; Angle5=90;
                    Angle2=75;  Angle6=90;
                    Angle3=15;  Angle7=60;
                    Angle4=105; Angle8=90;
                    break;
                case 2:
                    Angle1=105; Angle5=90;
                    Angle2=15;  Angle6=120;
                    Angle3=75;  Angle7=90;
                    Angle4=105; Angle8=90;
                    break;
                case 3:
                    Angle1=105; Angle5=90;
                    Angle2=75;  Angle6=90;
                    Angle3=75;  Angle7=90;
                    Angle4=165; Angle8=120;
                    break;
                case 4:
                    Angle1=165; Angle5=60;
                    Angle2=75;  Angle6=90;
                    Angle3=75;  Angle7=90;
                    Angle4=105; Angle8=90;
                    break;
            }
        }

        // 后退步态
        if (gaitMode == GAIT_BACKWARD)
        {
            gaitDelayCnt++;
            if (gaitDelayCnt >= GAIT_DELAY_BACKWARD)
            {
                gaitDelayCnt = 0;
                if (gaitStep > 4) gaitStep = 1;
                else gaitStep++;
            }
            switch(gaitStep)
            {
                case 1:
                    Angle1=165; Angle5=60;
                    Angle2=75;  Angle6=90;
                    Angle3=75;  Angle7=90;
                    Angle4=105; Angle8=90;
                    break;
                case 2:
                    Angle1=105; Angle5=90;
                    Angle2=75;  Angle6=90;
                    Angle3=75;  Angle7=90;
                    Angle4=165; Angle8=120;
                    break;
                case 3:
                    Angle1=105; Angle5=90;
                    Angle2=15;  Angle6=120;
                    Angle3=75;  Angle7=90;
                    Angle4=105; Angle8=90;
                    break;
                case 4:
                    Angle1=105; Angle5=90;
                    Angle2=75;  Angle6=90;
                    Angle3=15;  Angle7=60;
                    Angle4=105; Angle8=90;
                    break;
            }
        }

        // 顺时针旋转
        if (gaitMode == GAIT_ROTATE_CW)
        {
            gaitDelayCnt++;
            if (gaitDelayCnt >= GAIT_DELAY_ROTATECW)
            {
                gaitDelayCnt = 0;
                if (gaitStep > 4) gaitStep = 1;
                else gaitStep++;
            }
            switch(gaitStep)
            {
                case 1:
                    Angle1=105; Angle5=90;  Angle2=15;  Angle6=90;
                    Angle3=15;  Angle7=60;  Angle4=165; Angle8=90;
                    break;
                case 2:
                    Angle1=165; Angle5=60;  Angle2=15;  Angle6=90;
                    Angle3=75;  Angle7=90;  Angle4=165; Angle8=90;
                    break;
                case 3:
                    Angle1=105; Angle5=90;  Angle2=15;  Angle6=90;
                    Angle3=75;  Angle7=90;  Angle4=105; Angle8=120;
                    break;
                case 4:
                    Angle1=105; Angle5=90;  Angle2=75;  Angle6=120;
                    Angle3=75;  Angle7=90;  Angle4=165; Angle8=90;
                    break;
            }
        }

        // 逆时针旋转
        if (gaitMode == GAIT_ROTATE_CCW)
        {
            gaitDelayCnt++;
            if (gaitDelayCnt >= GAIT_DELAY_ROTATECCW)
            {
                gaitDelayCnt = 0;
                if (gaitStep > 4) gaitStep = 1;
                else gaitStep++;
            }
            switch(gaitStep)
            {
                case 1:
                    Angle1=165; Angle5=90;  Angle2=75;  Angle6=90;
                    Angle3=15;  Angle7=90;  Angle4=165; Angle8=120;
                    break;
                case 2:
                    Angle1=165; Angle5=90;  Angle2=15;  Angle6=120;
                    Angle3=15;  Angle7=90;  Angle4=105; Angle8=90;
                    break;
                case 3:
                    Angle1=165; Angle5=90;  Angle2=75;  Angle6=90;
                    Angle3=75;  Angle7=60;  Angle4=105; Angle8=90;
                    break;
                case 4:
                    Angle1=105; Angle5=60;  Angle2=75;  Angle6=90;
                    Angle3=15;  Angle7=90;  Angle4=105; Angle8=90;
                    break;
            }
        }

        // 右转
        if (gaitMode == GAIT_TURN_RIGHT)
        {
            gaitDelayCnt++;
            if (gaitDelayCnt >= GAIT_DELAY_TURNRIGHT)
            {
                gaitDelayCnt = 0;
                if (gaitStep > 4) gaitStep = 1;
                else gaitStep++;
            }
            switch(gaitStep)
            {
                case 1:
                    Angle1=120; Angle5=90;  Angle2=75;  Angle6=90;
                    Angle3=30;  Angle7=60;  Angle4=105; Angle8=90;
                    break;
                case 2:
                    Angle1=120; Angle5=90;  Angle2=15;  Angle6=120;
                    Angle3=60;  Angle7=90;  Angle4=105; Angle8=90;
                    break;
                case 3:
                    Angle1=120; Angle5=90;  Angle2=75;  Angle6=90;
                    Angle3=60;  Angle7=90;  Angle4=165; Angle8=120;
                    break;
                case 4:
                    Angle1=150; Angle5=60;  Angle2=75;  Angle6=90;
                    Angle3=60;  Angle7=90;  Angle4=105; Angle8=90;
                    break;
            }
        }

        // 左转
        if (gaitMode == GAIT_TURN_LEFT)
        {
            gaitDelayCnt++;
            if (gaitDelayCnt >= GAIT_DELAY_TURNLEFT)
            {
                gaitDelayCnt = 0;
                if (gaitStep > 4) gaitStep = 1;
                else gaitStep++;
            }
            switch(gaitStep)
            {
                case 1:
                    Angle1=105; Angle5=90;  Angle2=60;  Angle6=90;
                    Angle3=15;  Angle7=60;  Angle4=120; Angle8=90;
                    break;
                case 2:
                    Angle1=105; Angle5=90;  Angle2=30;  Angle6=120;
                    Angle3=75;  Angle7=90;  Angle4=120; Angle8=90;
                    break;
                case 3:
                    Angle1=105; Angle5=90;  Angle2=60;  Angle6=90;
                    Angle3=75;  Angle7=90;  Angle4=150; Angle8=120;
                    break;
                case 4:
                    Angle1=165; Angle5=60;  Angle2=60;  Angle6=90;
                    Angle3=75;  Angle7=90;  Angle4=120; Angle8=90;
                    break;
            }
        }

        // 蹲下
        if (gaitMode == GAIT_SIT)
        {
            gaitDelayCnt++;
            if (gaitDelayCnt >= GAIT_DELAY_SIT)
            {
                gaitDelayCnt = 0;
                if (gaitStep < 2) gaitStep++;
            }
            switch(gaitStep)
            {
                case 1:
                    Angle1=128; Angle5=90;
                    Angle2=52;  Angle6=90;
                    Angle3=52;  Angle7=65;
                    Angle4=128; Angle8=115;
                    break;
                case 2:
                    Angle1=120; Angle5=75;
                    Angle2=60;  Angle6=105;
                    Angle3=60;  Angle7=45;
                    Angle4=120; Angle8=135;
                    break;
            }
        }

        xSemaphoreGive(angleMutex);
        vTaskDelay(pdMS_TO_TICKS(25));
        }
    }
}


void StartSensorTask(void * argument)
{
    for(;;)
    {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET)
        {
            if (Huadong_State == 0)
            {
                gaitMode = GAIT_IDLE;
                Huadong_State = 1;
                Huadong_Step = 0;
                step_delay = 0;
                HW_ON();        // 拿起 → 蜂鸣器响
            }
            huadong_release_flag = 1;
        }
        else
        {
            if (huadong_release_flag == 1)
            {
                SetStandPosture();
                huadong_release_flag = 0;
                HW_OFF();       // 放下 → 蜂鸣器关
            }
            Huadong_State = 0;
        }

        if (Huadong_State != 0)
        {
            Huadong_Smooth();
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}


// ── 软件定时器回调 ──
// OLED 每 200ms 刷新一次（代替原来 StartDisplayTask 的手动计数）
void OledRefreshCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    OLED_Update();
}

// 板载 LED 每 300ms 交替闪烁（LED1亮→LED2亮→循环）
void LedBlinkCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    static uint8_t alt = 0;
    if (alt) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);  // LED1 ON
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);    // LED2 OFF
    } else {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);    // LED1 OFF
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);  // LED2 ON
    }
    alt = !alt;
}


void StartLedTask(void * argument)
{
    // 初始延迟，等 StartDefaultTask 完成开机闪灯（闪灯约需 2s）
    vTaskDelay(pdMS_TO_TICKS(3000));
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint16_t step = 0;

    for (;;) {
        switch (ledMode) {
            case LED_MODE_RAINBOW: {
                uint32_t grb = WS2812B_GrayToGRB((uint8_t)step);
                uint8_t r = (grb >> 8) & 0xFF;
                uint8_t g = (grb >> 16) & 0xFF;
                uint8_t b = grb & 0xFF;
                for (uint16_t j = 0; j < LED_NUM; j++)
                    WS2812B_SetPixel(j, r, g, b);
                WS2812B_Update();
                step = (step + 1) & 0xFF;
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(8));
                break;
            }
            default:  // LED_MODE_OFF
                for (uint16_t j = 0; j < LED_NUM; j++)
                    WS2812B_SetPixel(j, 0, 0, 0);
                WS2812B_Update();
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}

void StartNRFTask(void * argument)
{
    uint8_t rxBuf[32];
    uint8_t nrf_ok = 0;
    int lx = 50, ry = 50;
    int deadband_lo = 35, deadband_hi = 65;
    GaitMode_t prevGait = GAIT_IDLE;
    uint32_t rxCount = 0;
    uint8_t prev_sw1 = 0, prev_sw2 = 0;
    uint8_t nrf_active = 0;  // 标记当前步态是否由NRF控制，防覆盖语音/蓝牙指令

    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("NRF: Initializing...\n");
    NRF24L01_Init();

    // 增加重试机制
    for (int retry = 0; retry < 10; retry++) {
        if (NRF24L01_Check() == 0) {
            printf("NRF: Detected!\n");
            NRF24L01_RX_Mode();
            nrf_ok = 1;
            break;
        }
        printf("NRF: Check failed (retry %d/10)\n", retry + 1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (!nrf_ok) {
        printf("NRF: Init FAILED - check wiring!\n");
    }

    for (;;)
    {
        if (nrf_ok && NRF24L01_RxPacket(rxBuf) == 0)
        {
            rxCount++;

            if (rxBuf[0] == 'L' && rxBuf[1] == 'X')
            {
                /* 解析控制轴: LX(位置2-3)=左X, RY(位置12-13)=右Y */
                lx = (rxBuf[2] - '0') * 10 + (rxBuf[3] - '0');
                ry = (rxBuf[12] - '0') * 10 + (rxBuf[13] - '0');

                /* 读取按键状态 (位置14=左键, 15=右键) */
                uint8_t sw1 = (rxBuf[14] == '1') ? 1 : 0;
                uint8_t sw2 = (rxBuf[15] == '1') ? 1 : 0;

                /* 右键按下 → 切换灯光 */
                if (sw2 && !prev_sw2)
                    ledMode = (ledMode == LED_MODE_RAINBOW) ? LED_MODE_OFF : LED_MODE_RAINBOW;
                prev_sw2 = sw2;

                /* 左键按下 → 执行自动演示 */
                if (sw1 && !prev_sw1) {
                    RunAutoDemo();
                }
                prev_sw1 = sw1;

                /* 摇杆控制逻辑：
                   右摇杆 Y → 前进/后退
                   右摇杆居中 + 左摇杆 X → 原地顺时针/逆时针旋转
                   右摇杆向前 + 左摇杆 X → 左转/右转（行进中转向）
                   nrf_active 防止摇杆回中时覆盖语音/蓝牙的指令
                */
                if (ry > deadband_hi) {
                    nrf_active = 1;
                    if (lx > deadband_hi)
                        gaitMode = GAIT_TURN_RIGHT;    // 右前 + 左左 = 右转
                    else if (lx < deadband_lo)
                        gaitMode = GAIT_TURN_LEFT;     // 右前 + 左右 = 左转
                    else
                        gaitMode = GAIT_FORWARD;       // 仅右前 = 前进
                }
                else if (ry < deadband_lo) {
                    nrf_active = 1;
                    gaitMode = GAIT_BACKWARD;           // 右后 = 后退
                }
                else if (lx > deadband_hi) {
                    nrf_active = 1;
                    gaitMode = GAIT_ROTATE_CW;          // 右中 + 左左 = 顺时针
                }
                else if (lx < deadband_lo) {
                    nrf_active = 1;
                    gaitMode = GAIT_ROTATE_CCW;         // 右中 + 左右 = 逆时针
                }
                else {
                    if (nrf_active) {
                        nrf_active = 0;
                        gaitMode = GAIT_IDLE;
                        xSemaphoreTake(angleMutex, portMAX_DELAY);
                        SetStandPosture();
                        xSemaphoreGive(angleMutex);
                    }
                }

                if (gaitMode != GAIT_IDLE && prevGait != gaitMode) {
                    gaitStep = 1;
                    gaitDelayCnt = 0;
                    xTaskNotifyGive(gaitTaskHandle);
                }
                prevGait = gaitMode;
            }

            // 收到数据就闪一下板载 LED (PC13) — 低电平点亮
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   // 灭
            vTaskDelay(pdMS_TO_TICKS(100));
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // 亮
        }
        else if (!nrf_ok)
        {
            // NRF 未检测到，板载LED常亮提示错误
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        }

        vTaskDelay(pdMS_TO_TICKS(25));
    }
}

/* USER CODE END Application */

