#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

#define RX_BUF_SIZE 32

extern uint8_t Serial_RxPacket[RX_BUF_SIZE]; // 接收缓冲区
extern uint8_t Serial_RxFlag;               // 接收完成标志位

void Serial_Init(void);                      // 串口+DMA初始化
void Serial_SendString(char *str);
	
#endif

