#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "stm32f10x.h"
#include <stdint.h>

/* NRF24L01 引脚 - SPI1 */
#define NRF_CE(x)   do{ x ? GPIO_SetBits(GPIOA, GPIO_Pin_3) : GPIO_ResetBits(GPIOA, GPIO_Pin_3); }while(0)
#define NRF_CSN(x)  do{ x ? GPIO_SetBits(GPIOA, GPIO_Pin_4) : GPIO_ResetBits(GPIOA, GPIO_Pin_4); }while(0)

/* 寄存器地址 */
#define NRF_READ_REG    0x00
#define NRF_WRITE_REG   0x20
#define RD_RX_PLOAD     0x61
#define WR_TX_PLOAD     0xA0
#define FLUSH_TX        0xE1
#define FLUSH_RX        0xE2
#define REUSE_TX_PL     0xE3
#define NOP             0xFF

#define CONFIG          0x00
#define EN_AA           0x01
#define EN_RXADDR       0x02
#define SETUP_AW        0x03
#define SETUP_RETR      0x04
#define RF_CH           0x05
#define RF_SETUP        0x06
#define STATUS          0x07
#define MAX_TX          0x10
#define TX_OK           0x20
#define RX_OK           0x40
#define OBSERVE_TX      0x08
#define CD              0x09
#define RX_ADDR_P0      0x0A
#define TX_ADDR         0x10
#define RX_PW_P0        0x11
#define NRF_FIFO_STATUS 0x17

#define TX_ADR_WIDTH    5
#define RX_ADR_WIDTH    5
#define TX_PLOAD_WIDTH  32
#define RX_PLOAD_WIDTH  32

void NRF24L01_Init(void);
uint8_t NRF24L01_Check(void);
void NRF24L01_TX_Mode(void);
void NRF24L01_SendPacket(uint8_t *txbuf);

#endif
