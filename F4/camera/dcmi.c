#include "dcmi.h"
#include "ov5640.h"
#include "stm32f4xx_dcmi.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_syscfg.h"

/* ============================================================
 *  DCMI 8-bit 管脚定义 (F407)
 * ============================================================
 *  DCMI_D0  →  PC6   (AF13)
 *  DCMI_D1  →  PC7   (AF13)
 *  DCMI_D2  →  PC8   (AF13)
 *  DCMI_D3  →  PC9   (AF13)
 *  DCMI_D4  →  PC11  (AF13)
 *  DCMI_D5  →  PB6   (AF13)
 *  DCMI_D6  →  PE5   (AF13)
 *  DCMI_D7  →  PE6   (AF13)
 *  DCMI_HSYNC → PA4  (AF13)
 *  DCMI_VSYNC → PB7  (AF13)
 *  DCMI_PCLK  → PA6  (AF13)
 * ============================================================ */

static void DCMI_GPIO_Init(void) {
    GPIO_InitTypeDef gpio;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA |
                           RCC_AHB1Periph_GPIOB |
                           RCC_AHB1Periph_GPIOC |
                           RCC_AHB1Periph_GPIOE, ENABLE);

    /* PA4 (HSYNC), PA6 (PCLK) */
    gpio.GPIO_Pin   = GPIO_Pin_4 | GPIO_Pin_6;
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_DCMI);

    /* PB6 (D5), PB7 (VSYNC) */
    gpio.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOB, &gpio);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_DCMI);

    /* PC6 (D0), PC7 (D1), PC8 (D2), PC9 (D3), PC11 (D4) */
    gpio.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 |
                      GPIO_Pin_9 | GPIO_Pin_11;
    GPIO_Init(GPIOC, &gpio);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6,  GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7,  GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource8,  GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource9,  GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_DCMI);

    /* PE5 (D6), PE6 (D7) */
    gpio.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6;
    GPIO_Init(GPIOE, &gpio);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource5, GPIO_AF_DCMI);
    GPIO_PinAFConfig(GPIOE, GPIO_PinSource6, GPIO_AF_DCMI);
}

/* ============================================================
 *  DCMI + DMA 初始化
 *  QQVGA 160x120 RGB565 → 38400 bytes
 *  DMA2 Stream1 Channel1 → DCMI
 * ============================================================ */

void DCMI_Init(void) {
    DCMI_InitTypeDef  dcmi;
    DMA_InitTypeDef   dma;

    DCMI_GPIO_Init();

    /* 使能 DCMI & DMA 时钟 */
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    /* ---- DMA2 Stream1 Channel1 → 接收DCMI数据 ---- */
    DMA_DeInit(DMA2_Stream1);
    DMA_StructInit(&dma);
    dma.DMA_Channel            = DMA_Channel_1;
    dma.DMA_PeripheralBaseAddr = (u32)&DCMI->DR;
    dma.DMA_Memory0BaseAddr    = (u32)cam_frame_buf;
    dma.DMA_DIR                = DMA_DIR_PeripheralToMemory;
    dma.DMA_BufferSize         = CAM_BUF_SIZE / 4;  /* 32-bit 传输 */
    dma.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    dma.DMA_MemoryDataSize     = DMA_MemoryDataSize_Word;
    dma.DMA_Mode               = DMA_Mode_Circular;  /* 循环模式 */
    dma.DMA_Priority           = DMA_Priority_High;
    dma.DMA_FIFOMode           = DMA_FIFOMode_Enable;
    dma.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full;
    dma.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream1, &dma);

    /* ---- DCMI 配置 ---- */
    DCMI_DeInit();
    DCMI_StructInit(&dcmi);
    dcmi.DCMI_CaptureMode      = DCMI_CaptureMode_Continuous;
    dcmi.DCMI_SynchroMode      = DCMI_SynchroMode_Hardware;
    dcmi.DCMI_PCKPolarity      = DCMI_PCKPolarity_Rising;
    dcmi.DCMI_VSPolarity       = DCMI_VSPolarity_High;
    dcmi.DCMI_HSPolarity       = DCMI_HSPolarity_High;
    dcmi.DCMI_CaptureRate      = DCMI_CaptureRate_All_Frame;
    dcmi.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_8b;
    DCMI_Init(&dcmi);

    /* 使能DCMI DMA传输 */
    DCMI_DMACmd(ENABLE);

    /* 帧中断 → DCMI_IT_Frame */
    DCMI_ITConfig(DCMI_IT_FRAME, ENABLE);

    /* NVIC */
    {
        NVIC_InitTypeDef nvic;
        nvic.NVIC_IRQChannel                   = DCMI_IRQn;
        nvic.NVIC_IRQChannelPreemptionPriority = 1;
        nvic.NVIC_IRQChannelSubPriority        = 0;
        nvic.NVIC_IRQChannelCmd                = ENABLE;
        NVIC_Init(&nvic);
    }
}

void DCMI_Start(void) {
    cam_frame_ready = 0;
    DMA_Cmd(DMA2_Stream1, ENABLE);
    DCMI_Cmd(ENABLE);
    DCMI_CaptureCmd(ENABLE);
}

void DCMI_Stop(void) {
    DCMI_CaptureCmd(DISABLE);
    DCMI_Cmd(DISABLE);
    DMA_Cmd(DMA2_Stream1, DISABLE);
}

/* ============================================================
 *  DCMI 帧中断 —— 每帧结束时触发
 * ============================================================ */

void DCMI_IRQHandler(void) {
    if(DCMI_GetITStatus(DCMI_IT_FRAME) != RESET) {
        DCMI_ClearITPendingBit(DCMI_IT_FRAME);
        cam_frame_ready = 1;
    }
}
