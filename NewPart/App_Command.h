#ifndef __APP_COMMAND_H
#define __APP_COMMAND_H

#include "stm32f10x.h"

// 对外只暴露这一个函数，把收到的串口字符串扔给它就行了
void App_Command_Parse(char *packet);

#endif

