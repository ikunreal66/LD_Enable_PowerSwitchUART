#include "stm32f10x.h"
#include "OLED.h"
#include "Laser_Iset.h"
#include "Serial.h"
#include "Delay.h"
#include "App_Command.h" 
#include "string.h"

int main(void)
{
    char HighStr[12];
    char LowStr[12];
    
    /* Hardware Init */
    OLED_Init();
    Dac_Dma2_Tim2_Init(); 
    Serial_Init();        
    Laser_EN_Init();      
       

    OLED_Clear();
    OLED_ShowString(3, 1, "Set:0       ");
    OLED_ShowString(4, 1, "Lazer: OFF  ");
    OLED_ShowString(3, 10, "G:1"); 

    while (1)
    {    
        // 核心改动：把串口处理完全交给引擎！
        if (Serial_RxFlag == 1)
        {
            // 只需要这一行，把字符串丢进去，它自己会查表执行！
            App_Command_Parse((char*)Serial_RxPacket); 
            
            // 清空缓冲区
            memset(Serial_RxPacket, 0, RX_BUF_SIZE);
            Serial_RxFlag = 0;
        }
    
        /* Update Telemetry Display */
        GetActualVoltageStr(HighStr, LowStr);
        OLED_ShowString(1, 1, HighStr);
        OLED_ShowString(2, 1, LowStr);
        
        Delay_ms(200); 
    }
}
