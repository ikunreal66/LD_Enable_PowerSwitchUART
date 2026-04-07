#ifndef __PGA204_H
#define __PGA204_H

#include "stm32f10x.h"

/* 硬件引脚定义 */
#define PGA204_GPIO_PORT    GPIOB
#define PGA204_GPIO_CLK     RCC_APB2Periph_GPIOB
#define PGA204_A1_PIN       GPIO_Pin_0  // 连接 PB0
#define PGA204_A0_PIN       GPIO_Pin_1  // 连接 PB1

/* 增益枚举定义 */
typedef enum {
    PGA204_GAIN_1    = 1,
    PGA204_GAIN_10   = 10,
    PGA204_GAIN_100  = 100,
    PGA204_GAIN_1000 = 1000
} PGA204_Gain_t;

/* 函数声明 */
void PGA204_Init(void);
void PGA204_SetGain(PGA204_Gain_t gain);

#endif
