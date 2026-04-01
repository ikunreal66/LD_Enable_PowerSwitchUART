#include "Serial.h"
#include <string.h>

uint8_t Serial_RxPacket[RX_BUF_SIZE];
uint8_t Serial_RxFlag = 0;

void Serial_Init(void)
{
    /* 1. 开启时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* 2. GPIO配置 (PA9 TX, PA10 RX) */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 3. DMA配置 (USART1_RX -> DMA1_Channel5) */
    DMA_InitTypeDef DMA_InitStructure;
    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)Serial_RxPacket;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = RX_BUF_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // 正常模式，处理完手动重置
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel5, ENABLE);

    /* 4. USART配置 */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    /* 5. 开启中断和DMA请求 */
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE); // 开启空闲中断
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE); // 开启串口DMA接收请求

    /* 6. NVIC中断优先级配置 */
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}


//void USART1_IRQHandler(void)
//{
//    /* Read SR and DR to clear ALL flags including ORE, FE, IDLE */
//    uint8_t stat = USART1->SR;
//    uint8_t data = USART1->DR; 

//    if (stat & USART_SR_IDLE) 
//    {
//        /* Disable DMA to read received length */
//        DMA_Cmd(DMA1_Channel5, DISABLE);
//        
//        /* Your logic to process data: Serial_RxPacket... */
//        Serial_RxFlag = 1;

//        /* Reset DMA for next transfer */
//        DMA_SetCurrDataCounter(DMA1_Channel5, RX_BUF_SIZE);
//        DMA_Cmd(DMA1_Channel5, ENABLE);
//    }
//}

void USART1_IRQHandler(void)
{
    uint8_t stat = USART1->SR;
    uint8_t dummy = USART1->DR; // Read DR anyway to clear flags

    if (stat & USART_SR_IDLE) 
    {
        /* 1. Stop DMA to calculate length */
        DMA_Cmd(DMA1_Channel5, DISABLE);
        
        /* 2. Calculate received length */
        uint16_t rxLen = RX_BUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel5);
        
        /* 3. CRITICAL: Force add Null-Terminator to prevent atoi crash */
        if (rxLen < RX_BUF_SIZE) {
            Serial_RxPacket[rxLen] = '\0'; 
        } else {
            Serial_RxPacket[RX_BUF_SIZE - 1] = '\0';
        }

        Serial_RxFlag = 1;

        /* 4. Reset DMA */
        DMA_SetCurrDataCounter(DMA1_Channel5, RX_BUF_SIZE);
        DMA_Cmd(DMA1_Channel5, ENABLE);
    }

    /* 5. Handle Overrun/Noise/Frame Error to prevent deadlock */
    if (stat & (USART_SR_ORE | USART_SR_NE | USART_SR_FE)) {
        dummy = USART1->SR;
        dummy = USART1->DR;
    }
}

/* Add a helper function to send strings back to PC */
void Serial_SendString(char *str)
{
    while (*str)
    {
        USART_SendData(USART1, *str++);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}


