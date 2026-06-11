#include "stm32f10x.h"
#include "OLED_Font.h"


#define OLED_I2C_TIMEOUT  10000

/* * 核心机制说明：
 * 创建一个 1025 字节的显存缓冲区。
 * OLED_Buffer[0] 固定为 0x40，代表后续传输的全是“数据（Data）”流。
 * OLED_Buffer[1] ~ OLED_Buffer[1024] 对应屏幕的 128x64 像素点（8层 * 128列）。
 */
uint8_t OLED_Buffer[1025];

/**
  * @brief  硬件 I2C2 与 DMA1 通道4 初始化
  * @note   PB10 -> SCL, PB11 -> SDA
  */
void OLED_I2C_Init(void)
{
    /* 1. 开启外设时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  // GPIOB 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);   // 硬件 I2C2 时钟
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);     // DMA1 时钟

    /* 2. 配置引脚：PB10和PB11必须配置为【复用开漏输出(AF_OD)】 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 3. 配置硬件 I2C2 参数 */ 
    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x30;              // 只要不是0或OLED地址即可
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 400000;             // 400KHz 快速I2C速率
    I2C_Init(I2C2, &I2C_InitStructure);
    I2C_Cmd(I2C2, ENABLE);

    /* 4. 配置 DMA1 Channel4 (I2C2_TX 专用固定通道) */
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&I2C2->DR; // 外设目的地址：I2C数据寄存器
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)OLED_Buffer;   // 内存源地址：显存缓冲区
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;              // 方向：内存 -> 外设
    DMA_InitStructure.DMA_BufferSize = 1025;                        // 传输长度（1字节控制 + 1024字节数据）
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;// 外设地址固定
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;          // 内存地址自增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                   // 单次模式
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;            // 中等优先级
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                    // 关闭内存到内存
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);

    /* 5. 开启 I2C2 的 DMA 发送请求触发源 */
    I2C_DMACmd(I2C2, ENABLE);

    /* 6. 初始化显存的控制头 */
    OLED_Buffer[0] = 0x40; // 0x40告诉SSD1306接下来全是连续的数据流
}

/**
  * @brief  OLED 阻塞式硬件写命令（用于屏幕初始化与坐标重置）
  */
void OLED_WriteCommand(uint8_t Command)
{
    uint32_t TimeOut;

    // 1. 发送起始位
    I2C_GenerateSTART(I2C2, ENABLE);
    TimeOut = OLED_I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if (--TimeOut == 0) return; // 超时直接退出，放弃发送
    }

    // 2. 发送从机地址
    I2C_Send7bitAddress(I2C2, 0x78, I2C_Direction_Transmitter);
    TimeOut = OLED_I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if (--TimeOut == 0) return;
    }

    // 3. 发送控制字节
    I2C_SendData(I2C2, 0x00);
    TimeOut = OLED_I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
    {
        if (--TimeOut == 0) return;
    }

    // 4. 发送命令实体
    I2C_SendData(I2C2, Command);
    TimeOut = OLED_I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        if (--TimeOut == 0) return;
    }

    // 5. 发送停止位
    I2C_GenerateSTOP(I2C2, ENABLE);
}


/**
  * @brief  核心函数：将本地显存 Buffer 一键刷新到 OLED 屏幕（通过 DMA 搬运）
  */
void OLED_Update(void)
{
    uint32_t TimeOut;

    // ... 前面的设定坐标范围代码保持不变 ...
    
    DMA_Cmd(DMA1_Channel4, DISABLE);        
    DMA_ClearFlag(DMA1_FLAG_TC4);           
    DMA_SetCurrDataCounter(DMA1_Channel4, 1025); 

    // 握手时序加入超时保护
    I2C_GenerateSTART(I2C2, ENABLE);
    TimeOut = OLED_I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if (--TimeOut == 0) return; // 屏幕不在，取消本次刷屏计划
    }

    I2C_Send7bitAddress(I2C2, 0x78, I2C_Direction_Transmitter);
    TimeOut = OLED_I2C_TIMEOUT;
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if (--TimeOut == 0) return;
    }

    // 握手成功，启动 DMA 搬砖
    DMA_Cmd(DMA1_Channel4, ENABLE);

    // 等待 DMA 搬完和总线释放，也必须加超时！
    TimeOut = OLED_I2C_TIMEOUT * 10; // 这里可以适当放长一点，因为搬运 1025 字节需要 20ms
    while (DMA_GetFlagStatus(DMA1_FLAG_TC4) == RESET)
    {
        if (--TimeOut == 0) return;
    }
    
    TimeOut = OLED_I2C_TIMEOUT;
    while (I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF) == RESET)
    {
        if (--TimeOut == 0) return;
    }
    
    I2C_GenerateSTOP(I2C2, ENABLE);
}
/**
  * @brief  OLED 清屏（操作本地显存，随后刷新）
  */
void OLED_Clear(void)
{  
    uint16_t i;
    for (i = 1; i <= 1024; i++)
    {
        OLED_Buffer[i] = 0x00; // 本地缓存全部清空
    }
    OLED_Update(); // 将清屏数据同步到物理屏幕
}

/**
  * @brief  在本地显存绘制单个字符
  * @note   此函数修改后不再产生任何 I2C 通信，仅修改内存，支持全板平滑兼容
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{         
    uint8_t i;
    uint8_t page1 = (Line - 1) * 2;       // 字符上半部分所在页 (0~7)
    uint8_t page2 = page1 + 1;            // 字符下半部分所在页
    uint8_t start_x = (Column - 1) * 8;   // 字符起始列坐标 (0~127)

    /* 将字模的 8 字节数据填入本地全局显存对应的物理映射地址中 */
    for (i = 0; i < 8; i++)
    {
        OLED_Buffer[1 + (page1 * 128) + start_x + i] = OLED_F8x16[Char - ' '][i];
        OLED_Buffer[1 + (page2 * 128) + start_x + i] = OLED_F8x16[Char - ' '][i + 8];
    }
}

/* ========================================================================= */
/* 以下所有高级显示函数（String, Num 等）由于内部完全依赖上方的 OLED_ShowChar， */
/* 因此不需要做任何修改，它们会自动将字符绘制进内存中！                         */
/* ========================================================================= */

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) { Result *= X; }
    return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)                            
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++)                            
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)                            
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10) { OLED_ShowChar(Line, Column + i, SingleNumber + '0'); }
        else { OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A'); }
    }
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)                            
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/**
  * @brief  OLED系统初始化
  */
void OLED_Init(void)
{
    uint32_t i, j;
    for (i = 0; i < 1000; i++) { for (j = 0; j < 1000; j++); } // 上电稳定延时
    
    OLED_I2C_Init();            // 硬件外设与DMA初始化
    
    OLED_WriteCommand(0xAE);    // 关闭显示
    OLED_WriteCommand(0xD5);    // 设置显示时钟分频比
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8);    // 设置多路复用率
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3);    // 设置显示偏移
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);    // 设置显示开始行
    OLED_WriteCommand(0xA1);    // 左右方向段重定向
    OLED_WriteCommand(0xC8);    // 上下COM反向扫描
    OLED_WriteCommand(0xDA);    // COM引脚硬件配置
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81);    // 对比度
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9);    // 预充电周期
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB);    // VCOMH电压
    OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4);    // 全屏输出使能
    OLED_WriteCommand(0xA6);    // 正常显示模式
    OLED_WriteCommand(0x8D);    // 开启电荷泵内部升压
    OLED_WriteCommand(0x14);

    /* ====== 核心修改：将存储寻址模式变更为【水平寻址模式】 ===== */
    OLED_WriteCommand(0x20); 
    OLED_WriteCommand(0x00); // 必须是 0x00 才能支持连续的 DMA 全屏大图倾泻

    OLED_WriteCommand(0xAF);    // 开启显示
    OLED_Clear();               // 初始全清
}
