#include "ads1252.h"
#include "OLED.h"
#include "serial.h"


volatile int32_t ads_result = 0;
volatile uint8_t ads_new_data_flag = 0;
volatile uint16_t SampleCounter = 0;   // 采样计数
volatile uint8_t  SamplingDone = 0;    // 采样完成标志位
volatile int32_t ads_buffer[100] = {0};               // 存放 100 组数据
volatile uint32_t raw_data = 0;
uint8_t TargetSamples = 10;

void ADS1252_Init()
{
	ADS1252_MCO_Init();
	ADS1252_Interrupt_Init();
}

void ADS1252_MCO_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // PA8 配置为复用推挽输出，输出时钟方波
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    RCC_MCOConfig(RCC_MCO_HSI); 
}

void ADS1252_Interrupt_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    // 初始化时让 SCLK 保持低电平
    GPIO_ResetBits(GPIOB, GPIO_Pin_13);
	
    // 1. 配置 PB14 为输入
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 2. 连接 EXTI14 到 PB14
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);

    // 3. 配置 EXTI14 为下降沿触发
    EXTI_InitStructure.EXTI_Line = EXTI_Line14;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // 捕捉准备好的那一刻
//    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; // 捕捉准备好的那一刻
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    // 4. 配置 NVIC 优先级
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn; // PB14 在这里
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 优先级设高一点
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


void EXTI15_10_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line14) != RESET) {
        // 只有没读够 100 组时才执行读取逻辑
        if (SamplingDone == 0 && SampleCounter < TargetSamples) {          
            // 延迟 
            for(volatile int d = 0; d < 20; d++); 
			raw_data = 0;
            for (int i = 0; i < 24; i++) {
                GPIOB->BSRR = GPIO_Pin_13; // SCLK HIGH
                __NOP(); __NOP(); __NOP();
                
                raw_data <<= 1;
                if (GPIOB->IDR & GPIO_Pin_14) raw_data |= 0x01;

                GPIOB->BRR = GPIO_Pin_13;  // SCLK LOW
                __NOP(); __NOP(); __NOP();
            }

            // 符号扩展
            if (raw_data & 0x800000) raw_data |= 0xFF000000;
            
            // 存入数组并累加计数
            ads_buffer[SampleCounter] = (int32_t)raw_data;
            SampleCounter++;

            // 如果读够了 100 组，立即关掉中断使能
            if (SampleCounter >= TargetSamples) {
                // 关闭中断使能（防止频繁进中断干扰打印和 OLED）
				
                EXTI->IMR &= ~(EXTI_Line14); 
                SamplingDone = 1;	

            }
        }

        EXTI_ClearITPendingBit(EXTI_Line14);
    }
}

