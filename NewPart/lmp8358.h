#ifndef __LMP8358_H
#define __LMP8358_H

#include "stm32f10x.h" // 根据你的芯片型号修改，如 f4xx

/* 增益枚举定义 */
typedef enum {
    LMP_GAIN_10   = 0, // 000
    LMP_GAIN_20   = 1, // 001
    LMP_GAIN_50   = 2, // 010
    LMP_GAIN_100  = 3, // 011
    LMP_GAIN_200  = 4, // 100
    LMP_GAIN_500  = 5, // 101
    LMP_GAIN_1000 = 6, // 110
//    LMP_USER_DEF  = 7  // 111 (谨慎使用，除非外接电阻)
} LMP8358_Gain_TypeDef;

/* 函数声明 */
void LMP8358_Init(void);
void LMP8358_SetGain(LMP8358_Gain_TypeDef gain);

#endif

