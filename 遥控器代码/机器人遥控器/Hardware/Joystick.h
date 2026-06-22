#ifndef __JOYSTICK_H
#define __JOYSTICK_H

#include <stdint.h>

/* 双摇杆模块引脚映射
   J1: 左摇杆 - X=PA0, Y=PA1, SW=PB1
   J2: 右摇杆 - X=PA2, Y=PB0, SW=PB11
*/

void Joystick_Init(void);

/* 读取四个轴：12位ADC值 0~4095 */
void Joystick_Read(uint16_t *j1_x, uint16_t *j1_y,
                   uint16_t *j2_x, uint16_t *j2_y);

/* 摇杆按键状态：返回1=按下, 0=松开 */
uint8_t Joystick_SW1_Pressed(void);
uint8_t Joystick_SW2_Pressed(void);

#endif
