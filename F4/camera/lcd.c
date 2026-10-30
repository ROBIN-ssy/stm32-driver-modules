#include "lcd.h"
#include "delay.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

/* ============================================================
 *  SPI2 硬件 + DMA
 *  PB13=SCK(AF5), PB15=MOSI(AF5), TX-only 模式
 *  PB14 留给 RST（GPIO推挽），MISO 不用
 *  DMA1_Stream4_Channel0 → SPI2_TX
 * ============================================================ */

#define SPI2_DR_ADDR    ((u32)&SPI2->DR)

/* ---- 手动 SPI 写一个字节 (命令/数据阶段用，不走DMA) ---- */
static void spi_write_byte(u8 data) {
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI2, data);
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET);
}

/* ---- 手动写命令/数据 ---- */
static void lcd_write_cmd(u8 cmd) {
    LCD_CS_CLR();
    LCD_RS_CLR();
    spi_write_byte(cmd);
    LCD_CS_SET();
}

static void lcd_write_data(u8 dat) {
    LCD_CS_CLR();
    LCD_RS_SET();
    spi_write_byte(dat);
    LCD_CS_SET();
}

/* ============================================================
 *  SPI2 + DMA 初始化
 * ============================================================ */

static void LCD_SPI_DMA_Init(void) {
    GPIO_InitTypeDef  gpio;
    SPI_InitTypeDef   spi;
    DMA_InitTypeDef   dma;

    /* ---- 时钟 ---- */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC |
                           RCC_AHB1Periph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    /* ---- PB13(SCK), PB15(MOSI) → AF5 ---- */
    gpio.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_15;
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpio);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);

    /* ---- PB14(RST): 普通推挽输出 ---- */
    gpio.GPIO_Pin   = GPIO_Pin_14;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpio);
    LCD_RST_SET();

    /* ---- PB12(CS), PB1(BLK): GPIO 输出 ---- */
    gpio.GPIO_Pin   = GPIO_Pin_1 | GPIO_Pin_12;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_100MHz;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpio);

    /* ---- PC5(RS/DC): GPIO 输出 ---- */
    gpio.GPIO_Pin   = GPIO_Pin_5;
    GPIO_Init(GPIOC, &gpio);

    /* 初始状态 */
    LCD_CS_SET();
    LCD_RS_SET();

    /* ---- SPI2: 主机, TX-only, 8-bit, CPOL=0,CPHA=0 ---- */
    SPI_I2S_DeInit(SPI2);
    SPI_StructInit(&spi);
    spi.SPI_Direction         = SPI_Direction_1Line_Tx;  /* 只发不收 */
    spi.SPI_Mode              = SPI_Mode_Master;
    spi.SPI_DataSize          = SPI_DataSize_8b;
    spi.SPI_CPOL              = SPI_CPOL_Low;
    spi.SPI_CPHA              = SPI_CPHA_1Edge;
    spi.SPI_NSS               = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; /* 42/4=10.5MHz */
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;
    SPI_Init(SPI2, &spi);
    SPI_Cmd(SPI2, ENABLE);

    /* ---- DMA1_Stream4_Channel0 → SPI2_TX ---- */
    DMA_DeInit(DMA1_Stream4);
    DMA_StructInit(&dma);
    dma.DMA_Channel            = DMA_Channel_0;
    dma.DMA_PeripheralBaseAddr = SPI2_DR_ADDR;
    dma.DMA_Memory0BaseAddr    = 0;  /* 临时，用时再配 */
    dma.DMA_DIR                = DMA_DIR_MemoryToPeripheral;
    dma.DMA_BufferSize         = 0;
    dma.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode               = DMA_Mode_Normal;     /* 单次传输 */
    dma.DMA_Priority           = DMA_Priority_High;
    dma.DMA_FIFOMode           = DMA_FIFOMode_Disable;
    dma.DMA_MemoryBurst        = DMA_MemoryBurst_Single;
    dma.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
    DMA_Init(DMA1_Stream4, &dma);

    /* 使能 SPI TX DMA */
    SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
}

/* ============================================================
 *  ILI9341 初始化序列
 * ============================================================ */

void LCD_Init(void) {
    LCD_SPI_DMA_Init();

    /* 硬件复位 */
    LCD_RST_CLR();
    delay_ms(10);
    LCD_RST_SET();
    delay_ms(120);

    /* ---- ILI9341 初始化序列 ---- */
    lcd_write_cmd(0x01);    /* 软件复位 */
    delay_ms(120);

    lcd_write_cmd(0xCF);
    lcd_write_data(0x00); lcd_write_data(0xC1); lcd_write_data(0x30);

    lcd_write_cmd(0xED);
    lcd_write_data(0x64); lcd_write_data(0x03);
    lcd_write_data(0x12); lcd_write_data(0x81);

    lcd_write_cmd(0xE8);
    lcd_write_data(0x85); lcd_write_data(0x00); lcd_write_data(0x78);

    lcd_write_cmd(0xCB);
    lcd_write_data(0x39); lcd_write_data(0x2C);
    lcd_write_data(0x00); lcd_write_data(0x34); lcd_write_data(0x02);

    lcd_write_cmd(0xF7); lcd_write_data(0x20);
    lcd_write_cmd(0xEA); lcd_write_data(0x00); lcd_write_data(0x00);

    lcd_write_cmd(0xC0); lcd_write_data(0x23);
    lcd_write_cmd(0xC1); lcd_write_data(0x10);
    lcd_write_cmd(0xC5); lcd_write_data(0x3E); lcd_write_data(0x28);
    lcd_write_cmd(0xC7); lcd_write_data(0x86);

    lcd_write_cmd(0x36); lcd_write_data(0x48);
    lcd_write_cmd(0x3A); lcd_write_data(0x55);

    lcd_write_cmd(0xB1); lcd_write_data(0x00); lcd_write_data(0x18);
    lcd_write_cmd(0xB6);
    lcd_write_data(0x08); lcd_write_data(0x82); lcd_write_data(0x27);

    lcd_write_cmd(0xF2); lcd_write_data(0x00);
    lcd_write_cmd(0x26); lcd_write_data(0x01);

    /* Gamma+ */
    lcd_write_cmd(0xE0);
    lcd_write_data(0x0F); lcd_write_data(0x31); lcd_write_data(0x2B);
    lcd_write_data(0x0C); lcd_write_data(0x0E); lcd_write_data(0x08);
    lcd_write_data(0x4E); lcd_write_data(0xF1); lcd_write_data(0x37);
    lcd_write_data(0x07); lcd_write_data(0x10); lcd_write_data(0x03);
    lcd_write_data(0x0E); lcd_write_data(0x09); lcd_write_data(0x00);

    /* Gamma- */
    lcd_write_cmd(0xE1);
    lcd_write_data(0x00); lcd_write_data(0x0E); lcd_write_data(0x14);
    lcd_write_data(0x03); lcd_write_data(0x11); lcd_write_data(0x07);
    lcd_write_data(0x31); lcd_write_data(0xC1); lcd_write_data(0x48);
    lcd_write_data(0x08); lcd_write_data(0x0F); lcd_write_data(0x0C);
    lcd_write_data(0x31); lcd_write_data(0x36); lcd_write_data(0x0F);

    lcd_write_cmd(0x11);    /* 退出睡眠 */
    delay_ms(120);
    lcd_write_cmd(0x29);    /* 开显示 */

    LCD_BLK_SET();
}

/* ============================================================
 *  设置显示窗口 (手动写，不走DMA)
 * ============================================================ */

void LCD_SetWindow(u16 x1, u16 y1, u16 x2, u16 y2) {
    lcd_write_cmd(0x2A);
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1 & 0xFF);
    lcd_write_data(x2 >> 8);
    lcd_write_data(x2 & 0xFF);

    lcd_write_cmd(0x2B);
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1 & 0xFF);
    lcd_write_data(y2 >> 8);
    lcd_write_data(y2 & 0xFF);

    lcd_write_cmd(0x2C);    /* 准备写GRAM */
}

/* ============================================================
 *  画点 / 填充 / 清屏 (手动逐个像素，用于少量绘图)
 * ============================================================ */

static void lcd_write_pixel(u16 color) {
    LCD_CS_CLR();
    LCD_RS_SET();
    spi_write_byte(color >> 8);
    spi_write_byte(color & 0xFF);
    LCD_CS_SET();
}

void LCD_DrawPoint(u16 x, u16 y, u16 color) {
    LCD_SetWindow(x, y, x, y);
    lcd_write_pixel(color);
}

void LCD_Fill(u16 x1, u16 y1, u16 x2, u16 y2, u16 color) {
    u32 n = (u32)(x2 - x1 + 1) * (y2 - y1 + 1);
    u8 hi = color >> 8, lo = color & 0xFF;
    if(n == 0) return;
    LCD_SetWindow(x1, y1, x2, y2);

    LCD_CS_CLR();
    LCD_RS_SET();
    while(n--) {
        spi_write_byte(hi);
        spi_write_byte(lo);
    }
    LCD_CS_SET();
}

void LCD_Clear(u16 color) {
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, color);
}

/* ============================================================
 *  DMA 一次性刷图（性能核心）
 *
 *  pixels: RGB565 像素数据 (w × h × 2 字节)
 *  一次性设窗 + DMA 全帧轰入
 *  10.5MHz SPI → 38,400 字节 ≈ 30ms/帧 ≈ 30fps
 * ============================================================ */

void LCD_ShowFrame_DMA(const u8 *pixels, u16 w, u16 h,
                       u16 offset_x, u16 offset_y)
{
    u32 total_bytes = (u32)w * h * 2;

    if(total_bytes == 0) return;

    /* 1. 设置显示窗口 */
    LCD_SetWindow(offset_x, offset_y, offset_x + w - 1, offset_y + h - 1);

    /* 2. CS=0, RS=1, 准备接收像素数据 */
    LCD_CS_CLR();
    LCD_RS_SET();

    /* 3. 配置 DMA 并启动 */
    DMA_Cmd(DMA1_Stream4, DISABLE);
    DMA1_Stream4->M0AR = (u32)pixels;
    DMA1_Stream4->NDTR = total_bytes;
    DMA_Cmd(DMA1_Stream4, ENABLE);

    /* 4. 等待 DMA 传输完成 */
    while(DMA_GetFlagStatus(DMA1_Stream4, DMA_FLAG_TCIF4) == RESET);
    DMA_ClearFlag(DMA1_Stream4, DMA_FLAG_TCIF4);

    /* 5. 等待 SPI 发送完毕 */
    while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET);

    LCD_CS_SET();
}
