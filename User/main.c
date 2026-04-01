#include "stm32f10x.h"
#include "OLED.h"
#include "Laser_Iset.h"
#include "Serial.h"
#include "Delay.h"
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    char HighStr[12];
    char LowStr[12];
    
    /* Hardware Init */
    OLED_Init();
    Dac_Dma2_Tim2_Init(); 
    Serial_Init();        

    /* Clear OLED Initial Screen */
    OLED_Clear();
	

	while (1)
	{	
		if (Serial_RxFlag == 1)
		{
			/* 1. Safety check: Ensure packet isn't empty */
			if (Serial_RxPacket[0] >= '0' && Serial_RxPacket[0] <= '9')
			{
				int val = atoi((char*)Serial_RxPacket);
				if (val >= 0 && val <= 4095)
				{
					Set_Dac_Amplitude((uint16_t)val);
					
					// Use a safer display method
					char msg[16];
					sprintf(msg, "Set:%d    ", val);
					OLED_ShowString(4, 1, msg);
					
					Serial_SendString("DAC Updated OK\r\n");
				}
			}
			else 
			{
				Serial_SendString("Invalid Input\r\n");
			}
			Serial_RxFlag = 0; 
		}
	
		GetActualVoltageStr(HighStr, LowStr);
		OLED_ShowString(1, 1, HighStr);
		OLED_ShowString(2, 1, LowStr);
		
		/* Reduced delay to prevent serial buffer overflow */
		Delay_ms(20); 
	}
}
