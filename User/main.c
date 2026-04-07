#include "stm32f10x.h"
#include "OLED.h"
#include "Laser_Iset.h"
#include "Serial.h"
#include "Delay.h"
#include "PGA204.h"
#include "App_Command.h" 
#include <string.h>
#include <stdio.h>

int main(void)
{
    char HighStr[12];
    char LowStr[12];
    char DisplayBuf[16]; // 用于格式化 OLED 字符串
    
    /* 硬件初始化 */
    OLED_Init();
    Dac_Dma2_Tim2_Init(); 
    Serial_Init();        
    Laser_EN_Init();      
    PGA204_Init();

    /* 初始界面绘制 */
    OLED_Clear();
    OLED_ShowString(1, 1, "0.00V");  // 第一行：电压
    OLED_ShowString(2, 1, "Set:0       "); // 第二行：DAC 设定值
    OLED_ShowString(3, 1, "G:1         "); // 第三行：增益
    OLED_ShowString(4, 1, "Laser: OFF  "); // 第四行：开关状态

    while (1)
    {    
        // 1. 串口指令解析 (修改变量并设置 Flag)
        if (Serial_RxFlag == 1)
        {
            App_Command_Parse((char*)Serial_RxPacket); 
            memset(Serial_RxPacket, 0, RX_BUF_SIZE);
            Serial_RxFlag = 0;
        }

        // 2. 只有当收到指令改变了数据时，才刷新对应的 OLED 区域
        if (g_UpdateUI_Flag == 1)
        {
            // 刷新 DAC 设定行
            sprintf(DisplayBuf, "Set:%-4d    ", g_CurrentDAC);
            OLED_ShowString(2, 1, DisplayBuf);
            
            // 刷新增益行
            sprintf(DisplayBuf, "G:%-4d      ", g_CurrentGain);
            OLED_ShowString(3, 1, DisplayBuf);

            // 更新激光状态行
            if(g_LaserState) OLED_ShowString(4, 1, "Laser: ON   ");
            else             OLED_ShowString(4, 1, "Laser: OFF  ");
            
            g_UpdateUI_Flag = 0; // 刷新完成，清空标志
        }

        // 3. 周期性显示遥测电压 (HighStr)
        // 这个函数内部应该已经包含 sprintf 逻辑
        GetActualVoltageStr(HighStr, LowStr); 
        OLED_ShowString(1, 1, HighStr);
        
        // 维持 200ms 刷新率，既保证流畅，也给总线留出空隙
        Delay_ms(200); 
    }
}
