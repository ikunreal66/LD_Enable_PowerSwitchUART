#include "lmp8358.h"

/**
 * @brief  初始化增益控制引脚 (PB0, PB1, PB12)
 */
void LMP8358_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    // 开启 GPIOB 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    // 配置 PB0, PB1, PB12 为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 默认设置为增益 10 (全低)
    LMP8358_SetGain(LMP_GAIN_10);
}

/**
 * @brief  设置 LMP8358 增益
 * @param  gain: 增益值枚举
 * @note   映射关系:  G0->PB12, G1->PB0, G2->PB1
 */
void LMP8358_SetGain(LMP8358_Gain_TypeDef gain) {
    // 1. 设置 G0 (最低位) -> PB12
    if (gain & 0x01) GPIO_SetBits(GPIOC, GPIO_Pin_10);
    else             GPIO_ResetBits(GPIOC, GPIO_Pin_10);
	
	// 2. 设置 G1 (中间位) -> PB0
    if (gain & 0x02) GPIO_SetBits(GPIOC, GPIO_Pin_11);
    else             GPIO_ResetBits(GPIOC, GPIO_Pin_11);

    // 3. 设置 G2 (最高位) -> PB1
    if (gain & 0x04) GPIO_SetBits(GPIOC, GPIO_Pin_12);
    else             GPIO_ResetBits(GPIOC, GPIO_Pin_12);

}
