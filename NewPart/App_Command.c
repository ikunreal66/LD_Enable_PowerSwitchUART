#include "App_Command.h"
#include "Serial.h"
#include "Laser_Iset.h"
#include "lmp8358.h"
#include <string.h>
#include <stdlib.h>

/* Global sync variables for main loop display */
uint16_t g_CurrentDAC = 0;
uint16_t g_CurrentGain = 1;
uint8_t  g_LaserState = 0;
uint8_t  g_UpdateUI_Flag = 0;


/* Function pointer type for handlers */
typedef void (*CmdHandler)(char *param);

/* Command mapping structure */
typedef struct {
    const char *cmd_name;
    CmdHandler handler;
} Command_Map_t;

// --- Command Handlers (Logic Only) ---

static void Cmd_Laser_On(char *param) {
    Laser_Enable();
    g_LaserState = 1;
    g_UpdateUI_Flag = 1; // Trigger UI refresh in main
    printf("Laser ON OK\r\n");
}

static void Cmd_Laser_Off(char *param) {
    Laser_Disable();
    g_LaserState = 0;
    g_UpdateUI_Flag = 1;
    printf("Laser OFF OK\r\n");
}

static void Cmd_Set_Gain(char *param) {
    int val = atoi(param);
    LMP8358_Gain_TypeDef gain_enum;
    uint8_t valid = 1;

    /* 根据 LMP8358 的规格验证增益并映射到枚举 */
    switch (val) {
        case 10:   gain_enum = LMP_GAIN_10;   break;
        case 20:   gain_enum = LMP_GAIN_20;   break;
        case 50:   gain_enum = LMP_GAIN_50;   break;
        case 100:  gain_enum = LMP_GAIN_100;  break;
        case 200:  gain_enum = LMP_GAIN_200;  break;
        case 500:  gain_enum = LMP_GAIN_500;  break;
        case 1000: gain_enum = LMP_GAIN_1000; break;
        default:   valid = 0;                 break;
    }

    if (valid) {
        LMP8358_SetGain(gain_enum);
        
        g_CurrentGain = (uint16_t)val;
        g_UpdateUI_Flag = 1;
        printf("Gain Updated\r\n");
    } else {
        printf("Err: Invalid Gain! (Use 10,20,50,100,200,500,1000)\r\n");
    }
}


static void Cmd_Set_Dac(char *param) {
    int val = atoi(param);
    /* Standard 12-bit DAC range check */
    if (val >= 0 && val <= 4095) {
        Set_Dac_Amplitude((uint16_t)val);
        g_CurrentDAC = (uint16_t)val;
        g_UpdateUI_Flag = 1;
        printf("DAC Updated\r\n");
    } else {
        printf("Err: DAC Range\r\n");
    }
}


static void Cmd_Sample_Start(char *param) {
    
	int val = atoi(param);
    if (val > 0 && val <= 30) { // 假设最大缓存 100
        TargetSamples = (uint8_t)val;
    } else {
        TargetSamples = 10; // 默认值
    }
//	printf("Sampling %d points...\r\n", TargetSamples);
    SampleCounter = 0;
    SamplingDone = 0;	
    EXTI_ClearITPendingBit(EXTI_Line14);
    EXTI->IMR |= EXTI_Line14;
    
}


// --- Command Lookup Table ---
static const Command_Map_t Cmd_Table[] = {
    {"ON",  Cmd_Laser_On},
    {"OFF", Cmd_Laser_Off},
    {"G",   Cmd_Set_Gain},
    {"D",   Cmd_Set_Dac},
	{"S",   Cmd_Sample_Start}
};

/**
 * @brief Parse input packet and execute corresponding handler
 */
void App_Command_Parse(char *packet) {
    for (int i = 0; i < (sizeof(Cmd_Table)/sizeof(Cmd_Table[0])); i++) {
        int len = strlen(Cmd_Table[i].cmd_name);
        /* Check if packet starts with command name */
        if (strncmp(packet, Cmd_Table[i].cmd_name, len) == 0) {
            Cmd_Table[i].handler(packet + len); // Pass remaining string as param
            return;
        }
    }
    printf("Err: Unknown Command\r\n");
}

