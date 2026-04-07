#ifndef __APP_COMMAND_H
#define __APP_COMMAND_H

#include "stm32f10x.h"

// --- 全局同步变量声明 ---
extern uint16_t g_CurrentDAC;    // 当前 DAC 设定值
extern uint16_t g_CurrentGain;   // 当前增益倍数
extern uint8_t  g_LaserState;    // 激光开关状态 (0:OFF, 1:ON)
extern uint8_t  g_UpdateUI_Flag; // UI 刷新请求标志 (1: 需要刷新)

// --- 函数声明 ---
void App_Command_Parse(char *packet);

#endif
