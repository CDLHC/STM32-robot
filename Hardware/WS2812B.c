/**
 * WS2812B 灯带驱动
 *
 * 硬件：PB7 → TIM4_CH2 → DMA1_Channel4
 * 原理：用 TIM4 的 PWM 输出每 1.25μs 一个脉冲，通过 DMA 在每个周期开始前
 *       修改 CCR2（占空比寄存器），编码出 WS2812B 协议需要的 0 码和 1 码。
 *
 * 为什么用 DMA？
 *   WS2812B 对时序要求严格（±150ns），CPU 不能被打断，如果用 GPIO 模拟
 *   就要关中断，影响系统实时性。DMA 自动搬运占空比值到 CCR，CPU 零负担。
 *
 * 灯珠多怎么办？
 *   原先用 while 轮询等 DMA 完成，灯珠少（≤16）时没问题。
 *   灯珠多了改用 DMA 完成中断 + 信号量：DMA 传完自动进中断发信号量，
 *   调用方阻塞等待（让出 CPU），不影响其他任务实时性。
 *
 * 面试常问："WS2812B 的驱动原理是什么？"
 *   答：TIM4 产生 800kHz 的 PWM 基频，DMA 在每个 PWM 周期更新 CCR2，
 *   通过不同的占空比区分 0 码（30/90≈33%）和 1 码（59/90≈66%），
 *   连续 24 个周期组成一个 LED 的 GRB 颜色数据，
 *   最后 >50μs 的低电平作为复位信号让所有 LED 锁存。
 */

#include "WS2812B.h"
#include "main.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

static SemaphoreHandle_t dmaDoneSem = NULL;

// ============================================================
// 时序参数（基于 72MHz 主频，TIM4 时钟 = 72MHz）
// ============================================================
#define PWM_PERIOD 90      // 90 ticks × 13.89ns = 1.25μs（WS2812B 基频 800kHz）
#define T0H        30      // 0 码高电平：30 × 13.89ns ≈ 417ns（规格 200~500ns）
#define T1H        59      // 1 码高电平：59 × 13.89ns ≈ 819ns（规格 550~850ns）
// T0L = 90-30 = 60 → 833ns（规格 650~950ns ✓）
// T1L = 90-59 = 31 → 431ns（规格 450~750ns ✓）

// 缓冲区：每颗 LED 24 个 PWM 占空比值（uint16_t），离线编码后一次性 DMA 发送
static uint16_t dma_buffer[LED_NUM * 24] = {0};

// ============================================================
// 初始化：配置 TIM4、DMA、GPIO
// ============================================================
void WS2812B_Init(uint16_t numLeds)
{
    (void)numLeds;

    // ── 时钟使能 ──
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    RCC->AHBENR  |= RCC_AHBENR_DMA1EN;

    // ── PB7：复用推挽输出 50MHz ──
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIOB->CRL &= ~(0xF << 28);
    GPIOB->CRL |=  (0xB << 28);

    // ── TIM4 基础配置 ──
    TIM4->PSC  = 0;
    TIM4->ARR  = PWM_PERIOD - 1;
    TIM4->CCR2 = 0;

    // PWM 模式 1
    TIM4->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE;
    TIM4->CCER  |= TIM_CCER_CC2E;
    TIM4->CR1   |= TIM_CR1_ARPE;

    // ── DMA1 通道 4 配置 ──
    DMA1_Channel4->CPAR  = (uint32_t)&TIM4->CCR2;
    DMA1_Channel4->CMAR  = (uint32_t)dma_buffer;
    DMA1_Channel4->CCR   = DMA_CCR_MINC
                         | DMA_CCR_DIR
                         | DMA_CCR_PSIZE_0
                         | DMA_CCR_MSIZE_0
                         | DMA_CCR_PL_1;

    // ── 启动 ──
    TIM4->DIER |= TIM_DIER_CC2DE;
    TIM4->CR1  |= TIM_CR1_CEN;

    // ── DMA 中断 ──
    dmaDoneSem = xSemaphoreCreateBinary();
    DMA1_Channel4->CCR |= DMA_CCR_TCIE;
    NVIC_SetPriority(DMA1_Channel4_IRQn, 6);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

// ============================================================
// 设置第 idx 颗灯的颜色（传入 R/G/B，自动编码到 dma_buffer）
// ============================================================
void WS2812B_SetPixel(uint16_t idx, uint8_t r, uint8_t g, uint8_t b)
{
    if (idx >= LED_NUM) return;

    uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;

    for (int i = 0; i < 24; i++) {
        dma_buffer[idx * 24 + i] = (color & (1 << (23 - i))) ? T1H : T0H;
    }
}

// ============================================================
// 把 dma_buffer 的数据通过 DMA 发送给 WS2812B
// ============================================================
void WS2812B_Update(void)
{
    configASSERT(dmaDoneSem != NULL);

    DMA1_Channel4->CCR   &= ~DMA_CCR_EN;
    DMA1_Channel4->CNDTR  = LED_NUM * 24;
    DMA1->IFCR = DMA_IFCR_CTCIF4;
    DMA1_Channel4->CCR   |= DMA_CCR_EN;

    xSemaphoreTake(dmaDoneSem, portMAX_DELAY);

    TIM4->CCR2 = 0;
    vTaskDelay(pdMS_TO_TICKS(1));
}

// ============================================================
// DMA 完成中断处理函数
// ============================================================
void WS2812B_IRQHandler(void)
{
    if (DMA1->ISR & DMA_ISR_TCIF4)
    {
        DMA1->IFCR = DMA_IFCR_CTCIF4;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(dmaDoneSem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// ============================================================
// 彩虹效果辅助函数
// ============================================================
static uint32_t GrayToGRB(uint8_t gray)
{
    gray = 0xFF - gray;
    if (gray < 85)
        return ((0xFF - 3 * gray) << 8) | (3 * gray);
    if (gray < 170) {
        gray -= 85;
        return ((3 * gray) << 16) | (0xFF - 3 * gray);
    }
    gray -= 170;
    return ((0xFF - 3 * gray) << 16) | ((3 * gray) << 8);
}

uint32_t WS2812B_GrayToGRB(uint8_t gray) { return GrayToGRB(gray); }

void WS2812B_Rainbow(uint16_t interval_ms)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (uint16_t i = 0; i <= 255; i++) {
        uint32_t grb = GrayToGRB(i);
        uint8_t r = (grb >> 8) & 0xFF, g = (grb >> 16) & 0xFF, b = grb & 0xFF;
        for (uint16_t j = 0; j < LED_NUM; j++) WS2812B_SetPixel(j, r, g, b);
        WS2812B_Update();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(interval_ms));
    }
}

void WS2812B_FlashRed(uint8_t times, uint16_t interval_ms)
{
    for (uint8_t f = 0; f < times; f++) {
        for (uint16_t j = 0; j < LED_NUM; j++) WS2812B_SetPixel(j, 255, 0, 0);
        WS2812B_Update();
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
        for (uint16_t j = 0; j < LED_NUM; j++) WS2812B_SetPixel(j, 0, 0, 0);
        WS2812B_Update();
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}
