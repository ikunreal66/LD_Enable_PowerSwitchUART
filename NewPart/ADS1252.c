#include "ads1252.h"
#include "OLED.h"
#include "serial.h"

// ===================== ADS1252 硬件引脚宏定义（仅需修改此处适配硬件）=====================
// MCO 主时钟输出引脚
#define ADS1252_MCO_GPIO_PORT        GPIOA
#define ADS1252_MCO_GPIO_PIN         GPIO_Pin_8
#define ADS1252_MCO_RCC_CLK          RCC_APB2Periph_GPIOA
#define ADS1252_MCO_SRC              RCC_MCO_HSE    // MCO时钟源

// ADS1252 串行时钟 SCLK 引脚
#define ADS1252_SCLK_GPIO_PORT       GPIOB
#define ADS1252_SCLK_GPIO_PIN        GPIO_Pin_13
#define ADS1252_SCLK_RCC_CLK         RCC_APB2Periph_GPIOB

// ADS1252 数据就绪 DRDY 引脚（外部中断输入）
#define ADS1252_DRDY_GPIO_PORT       GPIOB
#define ADS1252_DRDY_GPIO_PIN        GPIO_Pin_14
#define ADS1252_DRDY_RCC_CLK         RCC_APB2Periph_GPIOB
#define ADS1252_DRDY_EXTI_LINE       EXTI_Line14
#define ADS1252_DRDY_EXTI_PORT_SRC   GPIO_PortSourceGPIOB
#define ADS1252_DRDY_EXTI_PIN_SRC    GPIO_PinSource14

// EXTI 复用功能时钟 + 中断通道
#define ADS1252_AFIO_RCC_CLK         RCC_APB2Periph_AFIO
#define ADS1252_EXTI_IRQn            EXTI15_10_IRQn

// 采样目标点数（业务参数也转为宏，方便配置）
#define ADS1252_TARGET_SAMPLES       10
// =========================================================================================

// 全局变量
volatile int32_t ads_result = 0;
volatile uint8_t ads_new_data_flag = 0;
volatile uint16_t SampleCounter = 0;   // 采样计数
volatile uint8_t  SamplingDone = 0;    // 采样完成标志位
volatile int32_t ads_buffer[100] = {0};// 存放 100 组数据
volatile uint32_t raw_data = 0;
uint8_t TargetSamples = ADS1252_TARGET_SAMPLES;


void ADS1252_Init()
{
	ADS1252_MCO_Init();
	ADS1252_Interrupt_Init();
}

/**
 * @brief  ADS1252 MCO时钟引脚初始化
 */
void ADS1252_MCO_Init(void)
{
    RCC_APB2PeriphClockCmd(ADS1252_MCO_RCC_CLK, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 配置MCO引脚为复用推挽输出
    GPIO_InitStructure.GPIO_Pin = ADS1252_MCO_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(ADS1252_MCO_GPIO_PORT, &GPIO_InitStructure);

    // 配置MCO时钟源
    RCC_MCOConfig(ADS1252_MCO_SRC); 
}

/**
 * @brief  ADS1252 SCLK + DRDY外部中断初始化
 */
void ADS1252_Interrupt_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // -------- 1. 初始化 SCLK 引脚(推挽输出) --------
    RCC_APB2PeriphClockCmd(ADS1252_SCLK_RCC_CLK, ENABLE);
	GPIO_InitStructure.GPIO_Pin = ADS1252_SCLK_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ADS1252_SCLK_GPIO_PORT, &GPIO_InitStructure);
    // 初始SCLK拉低
    GPIO_ResetBits(ADS1252_SCLK_GPIO_PORT, ADS1252_SCLK_GPIO_PIN);
	
    // -------- 2. 初始化 DRDY 引脚(上拉输入 + 外部中断) --------
    // 开启GPIOB + AFIO时钟
    RCC_APB2PeriphClockCmd(ADS1252_DRDY_RCC_CLK | ADS1252_AFIO_RCC_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin = ADS1252_DRDY_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_Init(ADS1252_DRDY_GPIO_PORT, &GPIO_InitStructure);

    // 引脚映射到EXTI线
    GPIO_EXTILineConfig(ADS1252_DRDY_EXTI_PORT_SRC, ADS1252_DRDY_EXTI_PIN_SRC);

    // 配置EXTI中断：下降沿触发
    EXTI_InitStructure.EXTI_Line = ADS1252_DRDY_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // 配置NVIC中断优先级
    NVIC_InitStructure.NVIC_IRQChannel = ADS1252_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief  DRDY外部中断服务函数，读取ADS1252 24bit数据
 */
void ADS1252_EXTI_IRQHandler(void)
{
    if (EXTI_GetITStatus(ADS1252_DRDY_EXTI_LINE) != RESET)
    {
        // 未采样完成且未采够点数，则读取数据
        if (SamplingDone == 0 && SampleCounter < TargetSamples)
        {          
            for(volatile int d = 0; d < 20; d++); 
			raw_data = 0;

            // 读取24位串行数据
            for (int i = 0; i < 24; i++)
            {
                // SCLK 置高
                ADS1252_SCLK_GPIO_PORT->BSRR = ADS1252_SCLK_GPIO_PIN;
                __NOP(); __NOP(); __NOP();__NOP();__NOP();
                
                raw_data <<= 1;
                // 读取DRDY引脚电平
                if (ADS1252_DRDY_GPIO_PORT->IDR & ADS1252_DRDY_GPIO_PIN)
                {
                    raw_data |= 0x01;
                }

                // SCLK 置低
                ADS1252_SCLK_GPIO_PORT->BRR = ADS1252_SCLK_GPIO_PIN;
                __NOP(); __NOP(); __NOP();__NOP();__NOP();
            }

            // 24位有符号数符号扩展
            if (raw_data & 0x800000)
            {
                raw_data |= 0xFF000000;
            }
            
            // 数据存入缓冲区
            ads_buffer[SampleCounter] = (int32_t)raw_data;
            SampleCounter++;

            // 采样点数达标，关闭中断、标记采样完成
            if (SampleCounter >= TargetSamples)
            {
                EXTI->IMR &= ~(ADS1252_DRDY_EXTI_LINE); 
                SamplingDone = 1;	
            }
        }

        // 清除中断标志位
        EXTI_ClearITPendingBit(ADS1252_DRDY_EXTI_LINE);
    }
}

// 中断函数名保持和工程一致（STM32标准库固定名）
void EXTI15_10_IRQHandler(void)
{
    ADS1252_EXTI_IRQHandler();
}

