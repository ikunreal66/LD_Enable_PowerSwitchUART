#include "stm32f10x.h"
#include "OLED.h"
#include "Laser_Iset.h"
#include "Serial.h"
#include "Delay.h"
#include <stdlib.h>
#include <string.h> // 必须包含这个，为了使用 strstr 字符串查找
#include <stdio.h>

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
    
    /* Display Initial State */
    OLED_ShowString(3, 1, "Set:0       ");
    OLED_ShowString(4, 1, "Lazer: OFF  ");

    while (1)
    {    
        if (Serial_RxFlag == 1)
        {
            /* 1. Command: Laser ON */
            if (strstr((char*)Serial_RxPacket, "ON") != NULL)
            {
                Laser_Enable();
                OLED_ShowString(4, 1, "State: ON "); 
                Serial_SendString("Laser ENABLED\r\n");
            }
            /* 2. Command: Laser OFF */
            else if (strstr((char*)Serial_RxPacket, "OFF") != NULL)
            {
                Laser_Disable();
                OLED_ShowString(4, 1, "State: OFF"); 
                Serial_SendString("Laser DISABLED\r\n");
            }
            /* 3. Command: Set DAC (Numeric only) */
            else if (Serial_RxPacket[0] >= '0' && Serial_RxPacket[0] <= '9')
            {
                int val = atoi((char*)Serial_RxPacket);
                if (val >= 0 && val <= 4095)
                {
                    Set_Dac_Amplitude((uint16_t)val);
                    
                    char msg[16];
                    sprintf(msg, "Set:%-4d    ", val);
                    OLED_ShowString(3, 1, msg);
                    
                    Serial_SendString("DAC Updated OK\r\n");
                }
                else
                {
                    Serial_SendString("DAC Out of Range\r\n");
                }
            }
            /* 4. Invalid Command */
            else 
            {
                Serial_SendString("Invalid Command\r\n");
            }
            
            /* Clear buffer and flag */
            memset(Serial_RxPacket, 0, RX_BUF_SIZE);
            Serial_RxFlag = 0;
        }
    
        /* Update Telemetry Display */
        GetActualVoltageStr(HighStr, LowStr);
        OLED_ShowString(1, 1, HighStr);
        OLED_ShowString(2, 1, LowStr);
        
        /* Loop delay to maintain stability */
        Delay_ms(200); 
    }
}
