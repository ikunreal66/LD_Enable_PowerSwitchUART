#ifndef __OLED_H
#define __OLED_H

#include <stdint.h> // 确保 uint8_t 等类型被正确识别




/* 初始化与硬件控制 */
void OLED_Init(void);          // OLED 系统初始化（内部已包含I2C和DMA初始化）
void OLED_Update(void);        // 【核心新增】将本地显存通过 DMA 一键刷新到屏幕

/* 基础显示与图形函数 */
void OLED_Clear(void);         // 清空本地显存（需配合 Update 才能在屏幕上消失）
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);

/* 数字显示函数 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif

