#include "stm32f10x.h"                  // Device header

/**
  * 函    数：信号检测初始化（EXTI外部中断 + TIM2计时器）
  * 参    数：无
  * 返 回 值：无
  */
void AD_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);		//开启AFIO的时钟，EXTI需要用到
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);		//开启TIM2的时钟
	
	/*GPIO初始化，PA0和PA3配置为上拉输入*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;				//上拉输入，无信号时为高电平
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA0和PA3引脚初始化为上拉输入
	
	/*AFIO中断引脚选择*/
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);	//PA0连接到EXTI0
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource3);	//PA3连接到EXTI3
	
	/*EXTI初始化，PA0和PA3都配置为下降沿触发*/
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line3;		//选择EXTI0和EXTI3线
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;					//使能
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;			//中断模式
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;		//下降沿触发，6V→0V时产生中断
	EXTI_Init(&EXTI_InitStructure);
	
	/*TIM2初始化，用于测量两个信号的间隔时间*/
	/*72MHz / 7200 = 10kHz，即0.1ms计数一次，最大6553.5ms*/
	TIM_InternalClockConfig(TIM2);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 65535;				//ARR，最大计数值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;			//PSC，72MHz/7200=10kHz
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						//清除初始化时产生的更新标志
	TIM_Cmd(TIM2, DISABLE);										//先不启动，等PA0中断时再启动
	
	/*NVIC配置*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;			//EXTI0中断（PA0，启动计时）
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;			//EXTI3中断（PA3，停止计时）
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&NVIC_InitStructure);
}
