#include "pga204.h"

/**
 * @brief  初始化 PGA204 控制引脚 (PB0, PB1)
 * @note   设置为推挽输出模式
 */
void PGA204_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. 开启 GPIOB 时钟 */
    RCC_APB2PeriphClockCmd(PGA204_GPIO_CLK, ENABLE);

    /* 2. 配置 PB0(A1) 和 PB1(A0) */
    GPIO_InitStructure.GPIO_Pin = PGA204_A1_PIN | PGA204_A0_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;    // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   // 输出速率
    GPIO_Init(PGA204_GPIO_PORT, &GPIO_InitStructure);

    /* 3. 初始状态设置为 1 倍增益 (A1=0, A0=0) */
    PGA204_SetGain(PGA204_GAIN_1);
}

/**
 * @brief  根据真值表切换增益
 * @param  gain: 目标增益倍数 (1, 10, 100, 1000)
 * @note   逻辑对应关系依据 PGA204 数据手册真值表
 */
void PGA204_SetGain(PGA204_Gain_t gain) {
    switch (gain) {
        case PGA204_GAIN_1:    // 1倍增益: A1=0, A0=0
            GPIO_ResetBits(PGA204_GPIO_PORT, PGA204_A1_PIN);
            GPIO_ResetBits(PGA204_GPIO_PORT, PGA204_A0_PIN);
            break;

        case PGA204_GAIN_10:   // 10倍增益: A1=0, A0=1
            GPIO_ResetBits(PGA204_GPIO_PORT, PGA204_A1_PIN);
            GPIO_SetBits(PGA204_GPIO_PORT, PGA204_A0_PIN);
            break;

        case PGA204_GAIN_100:  // 100倍增益: A1=1, A0=0
            GPIO_SetBits(PGA204_GPIO_PORT, PGA204_A1_PIN);
            GPIO_ResetBits(PGA204_GPIO_PORT, PGA204_A0_PIN);
            break;

        case PGA204_GAIN_1000: // 1000倍增益: A1=1, A0=1
            GPIO_SetBits(PGA204_GPIO_PORT, PGA204_A1_PIN);
            GPIO_SetBits(PGA204_GPIO_PORT, PGA204_A0_PIN);
            break;

        default:
            // 默认回退到 1 倍增益，确保安全
            GPIO_ResetBits(PGA204_GPIO_PORT, PGA204_A1_PIN);
            GPIO_ResetBits(PGA204_GPIO_PORT, PGA204_A0_PIN);
            break;
    }
}
