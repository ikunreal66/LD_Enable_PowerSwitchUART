#include "Serial.h"
#include <string.h>

uint8_t Serial_RxPacket[RX_BUF_SIZE];
uint8_t Serial_RxFlag = 0;

void Serial_Init(void)
{
    /* 1. Enable Clocks */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* 2. GPIO Config (PA9 TX, PA10 RX) */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // Alternate Function Push-Pull
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   // Input Pull-Up
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 3. DMA Config (USART1_RX -> DMA1_CH5) */
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
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel5, ENABLE);

    /* 4. USART Config */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    /* 5. Enable IT and DMA Request */
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE); // Idle Line Interrupt
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE); // Enable USART DMA RX

    /* 6. NVIC Config */
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

void USART1_IRQHandler(void)
{
    uint16_t stat = USART1->SR;
    uint16_t dummy = USART1->DR;

    /* 1. Error Handling: Clear ORE/NE/FE/PE flags */
    if (stat & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE))
    {
        dummy = USART1->SR;
        dummy = USART1->DR;
        DMA_Cmd(DMA1_Channel5, DISABLE);
        DMA_SetCurrDataCounter(DMA1_Channel5, RX_BUF_SIZE);
        DMA_Cmd(DMA1_Channel5, ENABLE);
        return; 
    }

    /* 2. Normal IDLE Handling */
    if (stat & USART_SR_IDLE) 
    {
        DMA_Cmd(DMA1_Channel5, DISABLE);
        
        uint16_t rxLen = RX_BUF_SIZE - DMA_GetCurrDataCounter(DMA1_Channel5);
        if (rxLen > 0 && rxLen < RX_BUF_SIZE) 
        {
            Serial_RxPacket[rxLen] = '\0'; // Null-Terminator
            Serial_RxFlag = 1; 
        }

        /* Reset DMA for next transfer */
        DMA1->IFCR = DMA1_IT_GL5; 
        DMA_SetCurrDataCounter(DMA1_Channel5, RX_BUF_SIZE);
        DMA_Cmd(DMA1_Channel5, ENABLE);
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


