#include "App_Command.h"
#include "Laser_Iset.h"
#include "OLED.h"
#include "Serial.h"
// #include "pga204.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ==========================================
 * 1. 定义函数指针类型
 * 所有的指令处理函数，都必须符合这种格式：无返回值，接收一个字符串参数
 * ========================================== */
typedef void (*CmdHandler)(char *param);

/* ==========================================
 * 2. 定义“指令字典”结构体
 * ========================================== */
typedef struct {
    const char *cmd_name;  // 指令名称 (如 "ON", "G", "D")
    CmdHandler handler;    // 对应的处理函数
} Command_Map_t;

/* ==========================================
 * 3. 具体的指令处理函数 (藏在内部，只专心做一件事)
 * ========================================== */

// 激光开
static void Cmd_Laser_On(char *param) {
    Laser_Enable();
    OLED_ShowString(4, 1, "Laser: ON ");
    Serial_SendString("Laser ENABLED\r\n");
}

// 激光关
static void Cmd_Laser_Off(char *param) {
    Laser_Disable();
    OLED_ShowString(4, 1, "Laser: OFF");
    Serial_SendString("Laser DISABLED\r\n");
}

// 设置增益 (例如收到 "G100"，传进来的 param 就是 "100")
// static void Cmd_Set_Gain(char *param) {
//     int g_val = atoi(param); 
//     if (g_val == 1 || g_val == 10 || g_val == 100 || g_val == 1000) {
//         PGA204_SetGain((PGA204_Gain_t)g_val);
//         char msg[10];
//         sprintf(msg, "G:%-4d", g_val);
//         OLED_ShowString(3, 10, msg);
//         Serial_SendString("Gain Updated OK\r\n");
//     } else {
//         Serial_SendString("Err: Gain Invalid\r\n");
//     }
// }

// 设置 DAC (例如收到 "D2048"，传进来的 param 就是 "2048")
static void Cmd_Set_Dac(char *param) {
    int val = atoi(param);
    if (val >= 0 && val <= 4095) {
        Set_Dac_Amplitude((uint16_t)val);
        char msg[16];
        sprintf(msg, "Set:%-4d    ", val);
        OLED_ShowString(3, 1, msg);
        Serial_SendString("DAC Updated OK\r\n");
    } else {
        Serial_SendString("Err: DAC Out of Range\r\n");
    }
}

/* ==========================================
 * 4. 建立终极查找表 (以后加功能，只需在这里加一行！)
 * ========================================== */
static const Command_Map_t Cmd_Table[] = {
    {"ON",   Cmd_Laser_On},   // 只要匹配到 "ON"，就执行 Cmd_Laser_On
    {"OFF",  Cmd_Laser_Off},
    // {"G",    Cmd_Set_Gain},   // 匹配到 "G"，执行 Cmd_Set_Gain
    {"D",    Cmd_Set_Dac},    // 匹配到 "D"，执行 Cmd_Set_Dac
};

// 计算表中有多少条指令
#define CMD_COUNT (sizeof(Cmd_Table) / sizeof(Cmd_Table[0]))


/* ==========================================
 * 5. 解析引擎：遍历查找表，自动调用函数 (对外接口)
 * ========================================== */
void App_Command_Parse(char *packet) {
    // 遍历字典中的每一个指令
    for (int i = 0; i < CMD_COUNT; i++) {
        int len = strlen(Cmd_Table[i].cmd_name);
        
        // 比较接收到的字符串前缀，是否和表里的名字一样
        // strncmp 比较前 len 个字符，相等返回 0
        if (strncmp(packet, Cmd_Table[i].cmd_name, len) == 0) {
            
            // 匹配成功！
            // packet + len 是指针偏移。
            // 比如 packet 是 "G100"，len 是 1，那传进去的就是 "100"
            Cmd_Table[i].handler(packet + len);
            
            return; // 执行完直接退出函数
        }
    }
    
    // 如果整个表都找遍了还没 return，说明是废指令
    Serial_SendString("Err: Unknown Command\r\n");
}



