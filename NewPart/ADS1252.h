#ifndef __ADS1252_H
#define __ADS1252_H

#include "stm32f10x.h"

extern volatile int32_t ads_result;
extern volatile uint8_t ads_new_data_flag;
extern volatile uint16_t SampleCounter;   // 采样计数
extern volatile uint8_t  SamplingDone;    // 采样完成标志位
extern volatile int32_t ads_buffer[100];
extern volatile uint32_t raw_data;
extern uint8_t TargetSamples;
	
void ADS1252_Init(void);
void ADS1252_MCO_Init(void);
void ADS1252_Interrupt_Init(void);
void EXTI15_10_IRQHandler(void);


#endif

