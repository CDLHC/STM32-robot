#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "LED.h"
#include "NRF24L01.h"
#include "Joystick.h"
#include <string.h>

/*
 * 四足机器人遥控器
 *
 * NRF24L01: SPI1 (PA5/6/7), CE=PA3, CSN=PA4
 * 摇杆: J1(PA0,PA1,PB1), J2(PA2,PB0,PB11)
 * LED: PC13 (板载)
 * OLED: PB8(SCL), PB9(SDA) - 软件I2C
 *
 * 数据格式: "LX00Y00RX00Y00" (32字节, 末尾补0)
 *   数值范围 10-90, 中心值=50
 */

int main(void)
{
    uint16_t j1x, j1y, j2x, j2y;
    uint8_t tx_buf[32];
    uint8_t nrf_ok = 0;

    OLED_Init();
    LED_Init();
    Joystick_Init();

    /* 初始化 NRF24L01 */
    NRF24L01_Init();
    if (NRF24L01_Check() == 0)
    {
        NRF24L01_TX_Mode();
        nrf_ok = 1;
        OLED_ShowString(1, 1, "NRF OK");
    }
    else
    {
        OLED_ShowString(1, 1, "NRF FAIL");
        while (1) { LED_Turn(); Delay_ms(200); }
    }

    while (1)
    {
        Joystick_Read(&j1x, &j1y, &j2x, &j2y);

        /* 摇杆转了90°，X/Y对调 */
        uint8_t lx = (uint8_t)((uint32_t)j1y * 80 / 4095) + 10;
        uint8_t ly = (uint8_t)((uint32_t)j1x * 80 / 4095) + 10;
        uint8_t rx = (uint8_t)((uint32_t)j2y * 80 / 4095) + 10;
        uint8_t ry = (uint8_t)((uint32_t)j2x * 80 / 4095) + 10;

        /* 组装数据包: "LXxxYxxRXxxYxx" */
        memset(tx_buf, 0, 32);
        tx_buf[0] = 'L'; tx_buf[1] = 'X';
        tx_buf[2] = '0' + lx / 10; tx_buf[3] = '0' + lx % 10;
        tx_buf[4] = 'Y';
        tx_buf[5] = '0' + ly / 10; tx_buf[6] = '0' + ly % 10;
        tx_buf[7] = 'R'; tx_buf[8] = 'X';
        tx_buf[9] = '0' + rx / 10; tx_buf[10] = '0' + rx % 10;
        tx_buf[11] = 'Y';
        tx_buf[12] = '0' + ry / 10; tx_buf[13] = '0' + ry % 10;
        tx_buf[14] = Joystick_SW1_Pressed() ? '1' : '0';  /* 左按键 */
        tx_buf[15] = Joystick_SW2_Pressed() ? '1' : '0';  /* 右按键 */

        NRF24L01_SendPacket(tx_buf);

        /* OLED 显示当前发送的数据 */
        OLED_ShowString(2, 1, (char*)tx_buf);
        OLED_ShowString(3, 1, "SW1:");
        OLED_ShowChar(3, 5, Joystick_SW1_Pressed() ? '1' : '0');
        OLED_ShowString(3, 7, "SW2:");
        OLED_ShowChar(3, 11, Joystick_SW2_Pressed() ? '1' : '0');
        LED_Turn();

        Delay_ms(50);
    }
}
