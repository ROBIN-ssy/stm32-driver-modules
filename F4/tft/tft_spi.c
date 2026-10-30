#include "tft_spi.h"
#include "stm32f4xx.h"

/* ============================================================
 *  SPI2 + DMA1 底层实现
 *
 *  SPI2_TX 的固定 DMA 映射：DMA1_Stream4 / Channel 0（F4 参考手册表 42）
 * ============================================================ */

#define TFT_SPIx           SPI2
#define TFT_DMA_STREAM     DMA1_Stream4
#define TFT_DMA_CHANNEL    DMA_Channel_0
#define TFT_DMA_TCIF       DMA_FLAG_TCIF4
#define TFT_DMA_ALLFLAGS   (DMA_FLAG_TCIF4 | DMA_FLAG_TEIF4 | DMA_FLAG_DMEIF4 | \
                            DMA_FLAG_FEIF4 | DMA_FLAG_HTIF4)

#define TFT_DMA_PSIZE_MASK (DMA_SxCR_PSIZE_0 | DMA_SxCR_PSIZE_1)
#define TFT_DMA_MSIZE_MASK (DMA_SxCR_MSIZE_0 | DMA_SxCR_MSIZE_1)

/* 等标志的超时上限：屏没接好也不会把主循环永久卡死 */
#define TFT_SPI_TIMEOUT    0x00200000U

static u8 s_init_done = 0U;

/* ---------------- 内部：帧宽度与 DMA 宽度切换 ---------------- */

/* 切到 8 位帧：命令、参数、以及 lcd.c 的逐字节写走这条路 */
static void tft_spi_frame8(void)
{
    if ((TFT_SPIx->CR1 & SPI_CR1_DFF) == 0U)
    {
        return;
    }
    TFT_SPI_WaitIdle();
    SPI_Cmd(TFT_SPIx, DISABLE);
    TFT_SPIx->CR1 = (u16)(TFT_SPIx->CR1 & (u16)(~SPI_CR1_DFF));
    SPI_Cmd(TFT_SPIx, ENABLE);
}

/* 切到 16 位帧：只有 DMA 批量发像素时才需要。
 * 必须是 16 位帧，硬件才会先发高字节，RGB565 的高低字节序才正确。 */
static void tft_spi_frame16(void)
{
    if ((TFT_SPIx->CR1 & SPI_CR1_DFF) != 0U)
    {
        return;
    }
    TFT_SPI_WaitIdle();
    SPI_Cmd(TFT_SPIx, DISABLE);
    TFT_SPIx->CR1 = (u16)(TFT_SPIx->CR1 | SPI_CR1_DFF);
    SPI_Cmd(TFT_SPIx, ENABLE);
}

/* PSIZE/MSIZE 只能在没有传输时改 */
static void tft_dma_set_datasize(u32 psize, u32 msize)
{
    DMA_Cmd(TFT_DMA_STREAM, DISABLE);
    TFT_DMA_STREAM->CR = (TFT_DMA_STREAM->CR &
                          ~(TFT_DMA_PSIZE_MASK | TFT_DMA_MSIZE_MASK)) |
                         psize | msize;
}

/* ---------------- 初始化 ---------------- */

void TFT_SPI_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef  spi;
    DMA_InitTypeDef  dma;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC |
                            RCC_AHB1Periph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    /* --- 片选 / DC / 背光：普通推挽输出 --- */
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;

    gpio.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_12;   /* BLK, CS */
    GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_5;                 /* DC */
    GPIO_Init(GPIOC, &gpio);

    TFT_CS_ROM();      /* 默认两个都不选，避免上电瞬间误触发字库 */
    TFT_DC_DATA();

    /* --- PB13/14/15 → 复用 AF5（SPI2） --- */
    gpio.GPIO_Pin  = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    gpio.GPIO_Mode = GPIO_Mode_AF;
    gpio.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpio);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);

    /* --- SPI2：Mode 0 / MSB first / 8 位帧 / 软件 NSS ---
     * ILI9341 与板载字库芯片都用 Mode 0，可以共用一个配置 */
    SPI_I2S_DeInit(TFT_SPIx);
    spi.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode              = SPI_Mode_Master;
    spi.SPI_DataSize          = SPI_DataSize_8b;
    spi.SPI_CPOL              = SPI_CPOL_Low;
    spi.SPI_CPHA              = SPI_CPHA_1Edge;
    spi.SPI_NSS               = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = TFT_SPI_BR_21M;
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial     = 7U;
    SPI_Init(TFT_SPIx, &spi);
    SPI_Cmd(TFT_SPIx, ENABLE);

    /* --- DMA1 Stream4 Ch0 → SPI2_TX --- */
    DMA_DeInit(TFT_DMA_STREAM);
    dma.DMA_Channel            = TFT_DMA_CHANNEL;
    dma.DMA_PeripheralBaseAddr = (u32)(&TFT_SPIx->DR);
    dma.DMA_Memory0BaseAddr    = (u32)0;
    dma.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    dma.DMA_BufferSize         = 0U;
    dma.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode               = DMA_Mode_Normal;
    dma.DMA_Priority           = DMA_Priority_VeryHigh;
    dma.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma.DMA_FIFOThreshold      = DMA_FIFOThreshold_HalfFull;
    dma.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    DMA_Init(TFT_DMA_STREAM, &dma);

    s_init_done = 1U;
}

void TFT_SPI_SetPrescaler(u16 prescaler)
{
    u16 cr1;

    TFT_SPI_WaitIdle();
    SPI_Cmd(TFT_SPIx, DISABLE);
    cr1 = (u16)((TFT_SPIx->CR1 & (u16)(~SPI_CR1_BR)) | prescaler);
    TFT_SPIx->CR1 = cr1;
    SPI_Cmd(TFT_SPIx, ENABLE);
}

/* ---------------- 阻塞收发 ---------------- */

void TFT_SPI_WaitIdle(void)
{
    u32 timeout;
    volatile u32 tmp;

    timeout = TFT_SPI_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(TFT_SPIx, SPI_I2S_FLAG_TXE) == RESET)
    {
        if (--timeout == 0U)
        {
            break;
        }
    }

    timeout = TFT_SPI_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(TFT_SPIx, SPI_I2S_FLAG_BSY) == SET)
    {
        if (--timeout == 0U)
        {
            break;
        }
    }

    /* 按参考手册的顺序清 OVR：先读 SR 再读 DR，顺便把 RXNE 一起清掉 */
    tmp = TFT_SPIx->SR;
    tmp = TFT_SPIx->DR;
    (void)tmp;
}

u8 TFT_SPI_ReadWriteByte(u8 tx)
{
    u32 timeout;

    timeout = TFT_SPI_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(TFT_SPIx, SPI_I2S_FLAG_TXE) == RESET)
    {
        if (--timeout == 0U)
        {
            return 0xFFU;
        }
    }
    SPI_I2S_SendData(TFT_SPIx, tx);

    timeout = TFT_SPI_TIMEOUT;
    while (SPI_I2S_GetFlagStatus(TFT_SPIx, SPI_I2S_FLAG_RXNE) == RESET)
    {
        if (--timeout == 0U)
        {
            return 0xFFU;
        }
    }
    return (u8)SPI_I2S_ReceiveData(TFT_SPIx);
}

void TFT_SPI_WriteByte(u8 tx)
{
    u32 timeout = TFT_SPI_TIMEOUT;

    while (SPI_I2S_GetFlagStatus(TFT_SPIx, SPI_I2S_FLAG_TXE) == RESET)
    {
        if (--timeout == 0U)
        {
            return;
        }
    }
    SPI_I2S_SendData(TFT_SPIx, tx);
}

void TFT_SPI_WriteBytes(const u8 *buf, u32 len)
{
    u32 i;

    if (buf == 0 || len == 0U)
    {
        return;
    }
    for (i = 0U; i < len; ++i)
    {
        TFT_SPI_WriteByte(buf[i]);
    }
    TFT_SPI_WaitIdle();
}

/* ---------------- DMA 批量 ---------------- */

/* 启动一次 DMA→SPI2_TX 并等它结束。nbytes 在字节宽度下是字节数，
 * 在半个字宽度下是传输次数（= 半字个数）。 */
static void tft_dma_send(const void *buf, u32 nbytes)
{
    u32 timeout;

    timeout = TFT_SPI_TIMEOUT;
    while (DMA_GetCmdStatus(TFT_DMA_STREAM) != DISABLE)
    {
        if (--timeout == 0U)
        {
            break;
        }
    }
    DMA_ClearFlag(TFT_DMA_STREAM, TFT_DMA_ALLFLAGS);

    TFT_DMA_STREAM->M0AR = (u32)buf;
    TFT_DMA_STREAM->NDTR = nbytes;

    SPI_I2S_DMACmd(TFT_SPIx, SPI_I2S_DMAReq_Tx, ENABLE);
    DMA_Cmd(TFT_DMA_STREAM, ENABLE);

    /* 等传输完成；阈值放大，覆盖大块像素流 */
    timeout = TFT_SPI_TIMEOUT * 8U;
    while (DMA_GetFlagStatus(TFT_DMA_STREAM, TFT_DMA_TCIF) == RESET)
    {
        if (--timeout == 0U)
        {
            break;
        }
    }

    DMA_Cmd(TFT_DMA_STREAM, DISABLE);
    SPI_I2S_DMACmd(TFT_SPIx, SPI_I2S_DMAReq_Tx, DISABLE);
    DMA_ClearFlag(TFT_DMA_STREAM, TFT_DMA_ALLFLAGS);
}

void TFT_SPI_WriteDMA(const u8 *buf, u32 len)
{
    if (s_init_done == 0U || buf == 0 || len == 0U)
    {
        return;
    }
    tft_spi_frame8();
    tft_dma_send(buf, len);
    TFT_SPI_WaitIdle();
}

void TFT_SPI_WriteDMA16(const u16 *buf, u32 count)
{
    if (s_init_done == 0U || buf == 0 || count == 0U)
    {
        return;
    }

    tft_spi_frame16();
    tft_dma_set_datasize(DMA_PeripheralDataSize_HalfWord,
                         DMA_MemoryDataSize_HalfWord);
    tft_dma_send(buf, count);
    TFT_SPI_WaitIdle();

    /* 恢复 8 位帧：紧接着的命令/参数都按字节发 */
    tft_dma_set_datasize(DMA_PeripheralDataSize_Byte,
                         DMA_MemoryDataSize_Byte);
    tft_spi_frame8();
}

u8 TFT_SPI_IsBusy(void)
{
    if (DMA_GetCmdStatus(TFT_DMA_STREAM) != DISABLE)
    {
        return 1U;
    }
    if (SPI_I2S_GetFlagStatus(TFT_SPIx, SPI_I2S_FLAG_BSY) == SET)
    {
        return 1U;
    }
    return 0U;
}
