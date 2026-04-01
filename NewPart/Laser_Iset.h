#ifndef __LASER_ISET_H
#define __LASER_ISET_H

/*
*LD Power is set by VLIS voltage.
*Five power levels: 1/16P, 1/8P, 1/4P, 1/2P, Full Power
*Corresponding VLIS voltage: 0.15625V, 0.3125V, 0.625V, 1.25V, 2.5V
*DACOUT Voltage = DACValue * 3300 / 4095
*/
#define VLIS_2V5        3102   // DAC输出2.5V 
#define VLIS_1V25       1551   // DAC输出1.25V
#define VLIS_0V625      776    // DAC输出0.625V
#define VLIS_0V3125     388    // DAC输出0.3125V
#define VLIS_0V15625    194    // DAC输出0.15625V


void Dac_Dma2_Tim2_Init(void);
static void ConvertToVolStr(uint16_t dacVal, char *buf);
void GetActualVoltageStr(char *hStr, char *lStr);
void Set_Dac_Amplitude(uint16_t v_high);
void Laser_EN_Init(void);
void Laser_Enable(void);
void Laser_Disable(void);



#endif
