#ifndef __WS2812B_H
#define __WS2812B_H

#include <stdint.h>

#define LED_NUM  16   // 你的灯带灯数

void WS2812B_Init(uint16_t numLeds);
void WS2812B_SetPixel(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);
void WS2812B_Update(void);
void WS2812B_Rainbow(uint16_t interval_ms);
void WS2812B_FlashRed(uint8_t times, uint16_t interval_ms);
uint32_t WS2812B_GrayToGRB(uint8_t gray);
void     WS2812B_IRQHandler(void);

#endif
