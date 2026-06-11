#include "stm32f10x.h"
#include "OLED.h"
#include "Laser_Iset.h"
#include "Serial.h"
#include "Delay.h"
#include "App_Command.h" 
#include "lmp8358.h"
#include <string.h>
#include <stdio.h>

int main(void)
{
    char DisplayBuf[16]; // 用于格式化 OLED 字符串
    //float AdcVoltage = 0.0f;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 必须设置优先级分组
    
	Serial_Init();
	OLED_Init();
	
	printf("=======OLED OK=======\r\n");
    Dac_Dma2_Tim2_Init();        
    Laser_EN_Init();
	Laser_Enable();
	LMP8358_Init();
	LMP8358_SetGain(LMP_GAIN_10);
	ADS1252_Init();
	
	
	g_CurrentDAC = 1000;   
    g_CurrentGain = 10;    
    g_LaserState = 1;      
    g_UpdateUI_Flag = 1;
	
	printf("=======All Init=======\r\n\r\n");
	
    while (1)
    {   
       // 1. 串口指令解析 (保持不变)
       if (Serial_RxFlag == 1)
       {
           App_Command_Parse((char*)Serial_RxPacket); 
           memset(Serial_RxPacket, 0, RX_BUF_SIZE);
           Serial_RxFlag = 0;
		   Delay_ms(10);
       }

/*========================== VOFA+ PRINT=================================================*/
		if (SamplingDone == 1) {
			for (int i = 0; i < TargetSamples; i++) {
				// 1. 获取原始数据并进行符号扩展（如果数组里存的是 int32_t，这里直接取即可）
				int32_t raw = ads_buffer[i];
		
				// 2. 换算为电压
				float voltage = (float)raw * (2.5f / 8388608.0f);
		
				// 3. 串口打印：打印索引号、十六进制原码和换算后的电压
				// 使用 %06X 打印 24 位十六进制，加上 0xFFFFFF 屏蔽位是为了只看低 24 位
				//printf("[%03d] Raw: 0x%06X | Volt: %.6f V\r\n", i, (unsigned int)(raw & 0xFFFFFF), voltage);
				//printf("--- End of Batch ---\r\n\r\n");
				
				printf(" %.6f\n", voltage);
			}	
			// 重置标志位，准备下一次触发
			SamplingDone = 0;
			SampleCounter = 0;
			g_UpdateUI_Flag = 1; 
		}
/*========================== VOFA+ PRINT END=======================================*/
       
		// 2. 界面标志位更新 
      if (g_UpdateUI_Flag == 1)
      {
          sprintf(DisplayBuf, "Set:%-4d    ", g_CurrentDAC);
          OLED_ShowString(2, 1, DisplayBuf);
          
          sprintf(DisplayBuf, "G:%-4d      ", g_CurrentGain);
          OLED_ShowString(3, 1, DisplayBuf);

          if(g_LaserState) OLED_ShowString(4, 1, "Laser: ON   ");
          else             OLED_ShowString(4, 1, "Laser: OFF  ");

			ConvertToVolStr(g_CurrentDAC, DisplayBuf);
			OLED_ShowString(1, 1, DisplayBuf);
			
		   
		    OLED_Update();
		   
           g_UpdateUI_Flag = 0; 
      }

		Delay_ms(10); 
    }
}

