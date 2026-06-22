#include "NRF24L01.h"
#include "spi.h"
#include "FreeRTOS.h"
#include "task.h"

/* 收发地址 - 必须与遥控器端一致 */
static const uint8_t TX_ADDRESS[TX_ADR_WIDTH] = {0x34,0x43,0x10,0x10,0x01};

/* SPI2读写一个字节 - 使用HAL，100ms超时防卡死 */
static uint8_t SPI2_ReadWriteByte(uint8_t dat)
{
    uint8_t rxdat;
    if (HAL_SPI_TransmitReceive(&hspi2, &dat, &rxdat, 1, 100) != HAL_OK)
        return 0xFF;
    return rxdat;
}

/* 写NRF寄存器 */
uint8_t NRF24L01_Write_Reg(uint8_t reg, uint8_t value)
{
    uint8_t status;
    NRF_CSN(0);
    status = SPI2_ReadWriteByte(reg);
    SPI2_ReadWriteByte(value);
    NRF_CSN(1);
    return status;
}

/* 读NRF寄存器 */
uint8_t NRF24L01_Read_Reg(uint8_t reg)
{
    uint8_t reg_val;
    NRF_CSN(0);
    SPI2_ReadWriteByte(reg);
    reg_val = SPI2_ReadWriteByte(0xFF);
    NRF_CSN(1);
    return reg_val;
}

/* 读NRF多字节缓冲区 */
uint8_t NRF24L01_Read_Buf(uint8_t reg, uint8_t *pBuf, uint8_t len)
{
    uint8_t status, i;
    NRF_CSN(0);
    status = SPI2_ReadWriteByte(reg);
    for (i = 0; i < len; i++)
        pBuf[i] = SPI2_ReadWriteByte(0xFF);
    NRF_CSN(1);
    return status;
}

/* 写NRF多字节缓冲区 */
uint8_t NRF24L01_Write_Buf(uint8_t reg, uint8_t *pBuf, uint8_t len)
{
    uint8_t status, i;
    NRF_CSN(0);
    status = SPI2_ReadWriteByte(reg);
    for (i = 0; i < len; i++)
        SPI2_ReadWriteByte(pBuf[i]);
    NRF_CSN(1);
    return status;
}

/* 初始化（GPIO和SPI2已由CubeMX在main中配置） */
void NRF24L01_Init(void)
{
    NRF_CE(0);
    NRF_CSN(1);
}

/* 检测NRF模块是否存在 */
uint8_t NRF24L01_Check(void)
{
    uint8_t buf[5] = {0xA5,0xA5,0xA5,0xA5,0xA5};
    uint8_t i;
    NRF24L01_Write_Buf(NRF_WRITE_REG + TX_ADDR, buf, 5);
    NRF24L01_Read_Buf(TX_ADDR, buf, 5);
    for (i = 0; i < 5; i++)
        if (buf[i] != 0xA5) return 1;
    return 0;
}

/* 配置为接收模式，与遥控器TX端匹配（channel 40, 2Mbps, 地址 0x34,0x43,0x10,0x10,0x01） */
void NRF24L01_RX_Mode(void)
{
    NRF_CE(0);                           // 拉低CE，进入待命状态，配置期间不接收

    // 写入接收管道0的地址，必须与遥控器发送地址一致才能收到数据
    NRF24L01_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, (uint8_t*)TX_ADDRESS, RX_ADR_WIDTH);

    NRF24L01_Write_Reg(NRF_WRITE_REG + EN_AA, 0x01);    // 使能管道0自动应答，确保丢包重传
    NRF24L01_Write_Reg(NRF_WRITE_REG + EN_RXADDR, 0x01); // 只使能管道0（只用了一个地址）
    NRF24L01_Write_Reg(NRF_WRITE_REG + RF_CH, 40);       // 射频信道 40 = 2.440GHz
    NRF24L01_Write_Reg(NRF_WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH); // 管道0负载宽度 = 32字节
    NRF24L01_Write_Reg(NRF_WRITE_REG + RF_SETUP, 0x0f);  // 2Mbps 通信速率，0dBm 发射功率
    NRF24L01_Write_Reg(NRF_WRITE_REG + CONFIG, 0x0f);    // 上电 + CRC使能(2字节) + 接收模式

    NRF_CE(1);                           // 拉高CE，进入接收状态，开始监听无线数据
    vTaskDelay(pdMS_TO_TICKS(1));         // 等待模块稳定进入RX模式（约130μs）
}

/* 接收一个数据包（非阻塞） */
uint8_t NRF24L01_RxPacket(uint8_t *rxbuf)
{
    uint8_t sta;
    sta = NRF24L01_Read_Reg(STATUS);
    NRF24L01_Write_Reg(NRF_WRITE_REG + STATUS, sta);

    if (sta & RX_OK) {
        NRF24L01_Read_Buf(RD_RX_PLOAD, rxbuf, RX_PLOAD_WIDTH);
        NRF24L01_Write_Reg(FLUSH_RX, 0xff);
        return 0;
    }
    return 1;
}
