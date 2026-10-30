#include "stm32f10x.h"                  // Device header
#include "Delay.h"

#define HX711_GPIO_CLK         	RCC_APB2Periph_GPIOA
#define HX711_SCK_GPIO_PORT    	GPIOB
#define HX711_SCK_GPIO_PIN		 	GPIO_Pin_9
#define HX711_DT_GPIO_PORT		 	GPIOB
#define HX711_DT_GPIO_PIN			 	GPIO_Pin_8

#define HX711_SCK_H			GPIO_SetBits(HX711_SCK_GPIO_PORT,HX711_SCK_GPIO_PIN);
#define HX711_SCK_L			GPIO_ResetBits(HX711_SCK_GPIO_PORT,HX711_SCK_GPIO_PIN);

#define HX711_DT				GPIO_ReadInputDataBit(HX711_DT_GPIO_PORT,HX711_DT_GPIO_PIN)

void HX711_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(HX711_GPIO_CLK,ENABLE);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = HX711_SCK_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(HX711_SCK_GPIO_PORT,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = HX711_DT_GPIO_PIN;
	GPIO_Init(HX711_DT_GPIO_PORT,&GPIO_InitStructure);
}

unsigned long HX711_GetData(void)
{
	unsigned long Count;
	unsigned char i;
	HX711_SCK_L;
	Delay_us(1);
	Count = 0;
	while(HX711_DT);
	for (i = 0;i < 24; i++)
	{
		HX711_SCK_H;
		Delay_us(1);
		Count = Count<<1;
		HX711_SCK_L;
		Delay_us(1);
		if(HX711_DT) Count++;
	}
	HX711_SCK_H;
	Delay_us(1);
	Count = Count^0x800000;
	HX711_SCK_L;
	return (Count);
}

