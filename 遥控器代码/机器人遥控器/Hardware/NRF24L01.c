#include "NRF24L01.h"
#include "Delay.h"

/* 收发地址 - 必须与机器人端一致 */
static const uint8_t TX_ADDRESS[TX_ADR_WIDTH] = {0x34,0x43,0x10,0x10,0x01};

/* SPI1 初始化 - 使用标准外设库 */
static void SPI1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct ;
    SPI_InitTypeDef SPI_InitStruct ;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA5 SCK, PA7 MOSI - 复用推挽输出 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA6 MISO - 浮空输入 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* SPI1: 主机模式, 9MHz (72/8), CPOL=0 CPHA=0, 高位先行 */
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_Init(SPI1, &SPI_InitStruct);

    SPI_Cmd(SPI1, ENABLE);
}

/* SPI1 读写一个字节 */
static uint8_t SPI1_ReadWriteByte(uint8_t dat)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, dat);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI1);
}

/* 写NRF寄存器 */
static uint8_t NRF24L01_Write_Reg(uint8_t reg, uint8_t value)
{
    uint8_t status;
    NRF_CSN(0);
    status = SPI1_ReadWriteByte(reg);
    SPI1_ReadWriteByte(value);
    NRF_CSN(1);
    return status;
}

/* 读NRF寄存器 */
static uint8_t NRF24L01_Read_Reg(uint8_t reg)
{
    uint8_t reg_val;
    NRF_CSN(0);
    SPI1_ReadWriteByte(reg);
    reg_val = SPI1_ReadWriteByte(0xFF);
    NRF_CSN(1);
    return reg_val;
}

/* 写NRF多字节缓冲区 */
static uint8_t NRF24L01_Write_Buf(uint8_t reg, uint8_t *pBuf, uint8_t len)
{
    uint8_t status, i;
    NRF_CSN(0);
    status = SPI1_ReadWriteByte(reg);
    for (i = 0; i < len; i++)
        SPI1_ReadWriteByte(pBuf[i]);
    NRF_CSN(1);
    return status;
}

/* 初始化NRF引脚和SPI */
void NRF24L01_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA3 = CE, PA4 = CSN */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    SPI1_Init();

    NRF_CE(0);//待机
    NRF_CSN(1);//高电平，关闭 SPI 片选
}

/* 检测NRF模块是否存在 */
uint8_t NRF24L01_Check(void)
{
    uint8_t buf[5] = {0xA5,0xA5,0xA5,0xA5,0xA5};
    uint8_t i;
    NRF24L01_Write_Buf(NRF_WRITE_REG + TX_ADDR, buf, 5);
    NRF24L01_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, buf, 5);
    for (i = 0; i < 5; i++)
        if (buf[i] != 0xA5) return 1;
    return 0;
}

/* 配置为发送模式 - channel 40, 2Mbps，与机器人接收端匹配 */
void NRF24L01_TX_Mode(void)
{
    NRF_CE(0);

    /* 设置发送地址 */
    NRF24L01_Write_Buf(NRF_WRITE_REG + TX_ADDR, (uint8_t*)TX_ADDRESS, TX_ADR_WIDTH);
    /* 设置自动应答接收地址（必须与发送地址相同） */
    NRF24L01_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, (uint8_t*)TX_ADDRESS, TX_ADR_WIDTH);

    /* 使能 pipe 0 自动应答 */
    NRF24L01_Write_Reg(NRF_WRITE_REG + EN_AA, 0x01);
    /* 只使能 pipe 0 */
    NRF24L01_Write_Reg(NRF_WRITE_REG + EN_RXADDR, 0x01);
    /* 地址宽度 5 字节 */
    NRF24L01_Write_Reg(NRF_WRITE_REG + SETUP_AW, 0x03);
    /* 自动重发：500us 间隔，3 次重发 */
    NRF24L01_Write_Reg(NRF_WRITE_REG + SETUP_RETR, 0x13);
    /* 射频信道 40 = 2.440GHz */
    NRF24L01_Write_Reg(NRF_WRITE_REG + RF_CH, 40);
    /* 2Mbps, 0dBm 发射功率 */
    NRF24L01_Write_Reg(NRF_WRITE_REG + RF_SETUP, 0x0F);
    /* 上电, CRC使能, CRC=2字节, 发送模式 */
    NRF24L01_Write_Reg(NRF_WRITE_REG + CONFIG, 0x0E);

    NRF_CE(1);
    Delay_ms(2);
}

/* 发送一个数据包 */
void NRF24L01_SendPacket(uint8_t *txbuf)
{
    uint8_t sta;

    NRF_CE(0);

    /* 将数据写入发送FIFO */
    NRF24L01_Write_Buf(WR_TX_PLOAD, txbuf, TX_PLOAD_WIDTH);

    NRF_CE(1);
    Delay_us(15);

    /* 等待发送完成或超时 */
    uint32_t timeout = 10000;
    do {
        sta = NRF24L01_Read_Reg(STATUS);
        if (--timeout == 0) break;
    } while ((sta & (TX_OK | MAX_TX)) == 0);

    /* 清除状态标志 */
    NRF24L01_Write_Reg(NRF_WRITE_REG + STATUS, sta);

    /* 达到最大重发次数时清空FIFO */
    if (sta & MAX_TX) {
        NRF24L01_Write_Reg(FLUSH_TX, 0xFF);
    }
}
