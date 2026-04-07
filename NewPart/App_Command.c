#include "App_Command.h"
#include "Serial.h"
#include "Laser_Iset.h"
#include "pga204.h"
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
    Serial_SendString("Laser ON OK\r\n");
}

static void Cmd_Laser_Off(char *param) {
    Laser_Disable();
    g_LaserState = 0;
    g_UpdateUI_Flag = 1;
    Serial_SendString("Laser OFF OK\r\n");
}

static void Cmd_Set_Gain(char *param) {
    int val = atoi(param);
    /* Validate gain values per PGA204 spec */
    if (val == 1 || val == 10 || val == 100 || val == 1000) {
        PGA204_SetGain((PGA204_Gain_t)val);
        g_CurrentGain = (uint16_t)val;
        g_UpdateUI_Flag = 1;
        Serial_SendString("Gain Updated\r\n");
    } else {
        Serial_SendString("Err: Gain Val\r\n");
    }
}

static void Cmd_Set_Dac(char *param) {
    int val = atoi(param);
    /* Standard 12-bit DAC range check */
    if (val >= 0 && val <= 4095) {
        Set_Dac_Amplitude((uint16_t)val);
        g_CurrentDAC = (uint16_t)val;
        g_UpdateUI_Flag = 1;
        Serial_SendString("DAC Updated\r\n");
    } else {
        Serial_SendString("Err: DAC Range\r\n");
    }
}

// --- Command Lookup Table ---
static const Command_Map_t Cmd_Table[] = {
    {"ON",  Cmd_Laser_On},
    {"OFF", Cmd_Laser_Off},
    {"G",   Cmd_Set_Gain},
    {"D",   Cmd_Set_Dac},
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
    Serial_SendString("Err: Unknown Command\r\n");
}

