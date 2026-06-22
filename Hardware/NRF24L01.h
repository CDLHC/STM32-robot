#ifndef __NRF24L01_H
#define __NRF24L01_H

#include "main.h"
#include <stdint.h>

/* NRF24L01 pins (robot: SPI2) */
#define NRF_CE(x)   do{ x ? HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET) : HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); }while(0)
#define NRF_CSN(x)  do{ x ? HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET) : HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET); }while(0)

/* Register definitions */
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
#define RX_ADDR_P1      0x0B
#define TX_ADDR         0x10
#define RX_PW_P0        0x11
#define NRF_FIFO_STATUS 0x17

#define TX_ADR_WIDTH    5
#define RX_ADR_WIDTH    5
#define TX_PLOAD_WIDTH  32
#define RX_PLOAD_WIDTH  32

/* Received command buffer (null-terminated after parsing) */
typedef struct {
    uint8_t buf[32];
    uint8_t len;
    uint8_t valid;
} NRF_Packet_t;

void NRF24L01_Init(void);
void NRF24L01_RX_Mode(void);
uint8_t NRF24L01_Read_Reg(uint8_t reg);
uint8_t NRF24L01_Write_Reg(uint8_t reg, uint8_t value);
uint8_t NRF24L01_Read_Buf(uint8_t reg, uint8_t *pBuf, uint8_t len);
uint8_t NRF24L01_Write_Buf(uint8_t reg, uint8_t *pBuf, uint8_t len);
uint8_t NRF24L01_Check(void);
uint8_t NRF24L01_RxPacket(uint8_t *rxbuf);

#endif
