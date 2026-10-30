#include "ov5640.h"
#include "delay.h"
#include <string.h>

/* ============================================================
 *  OV5640 管脚定义 (软I2C + 控制)
 * ============================================================ */
#define OV_SCCB_SCL_PIN   GPIO_Pin_10    /* PB10 */
#define OV_SCCB_SDA_PIN   GPIO_Pin_11    /* PB11 */
#define OV_SCCB_PORT      GPIOB
#define OV_SCCB_RCC       RCC_AHB1Periph_GPIOB

#define OV_RST_PIN        GPIO_Pin_15    /* PG15 */
#define OV_RST_PORT       GPIOG
#define OV_RST_RCC        RCC_AHB1Periph_GPIOG

#define SCCB_SDA_IN()   {GPIOB->MODER&=~(3<<(11*2));GPIOB->MODER|=0<<(11*2);}
#define SCCB_SDA_OUT()  {GPIOB->MODER&=~(3<<(11*2));GPIOB->MODER|=1<<(11*2);}
#define SCCB_SCL_H()    GPIO_SetBits(OV_SCCB_PORT, OV_SCCB_SCL_PIN)
#define SCCB_SCL_L()    GPIO_ResetBits(OV_SCCB_PORT, OV_SCCB_SCL_PIN)
#define SCCB_SDA_H()    GPIO_SetBits(OV_SCCB_PORT, OV_SCCB_SDA_PIN)
#define SCCB_SDA_L()    GPIO_ResetBits(OV_SCCB_PORT, OV_SCCB_SDA_PIN)
#define SCCB_SDA_READ() GPIO_ReadInputDataBit(OV_SCCB_PORT, OV_SCCB_SDA_PIN)

/* 帧缓冲区 */
u8  cam_frame_buf[CAM_BUF_SIZE] __attribute__((aligned(4)));
u8  cam_frame_ready = 0;

static u8 sccb_delay_t = 4;

/* ============================================================
 *  SCCB (软件I2C) 基础操作
 * ============================================================ */

static void sccb_delay(void) {
    volatile u8 i = sccb_delay_t;
    while(i--) __NOP();
}

static void sccb_start(void) {
    SCCB_SDA_OUT();
    SCCB_SDA_H();
    SCCB_SCL_H();
    sccb_delay();
    SCCB_SDA_L();
    sccb_delay();
    SCCB_SCL_L();
}

static void sccb_stop(void) {
    SCCB_SDA_OUT();
    SCCB_SDA_L();
    SCCB_SCL_H();
    sccb_delay();
    SCCB_SDA_H();
    sccb_delay();
}

static u8 sccb_wait_ack(void) {
    u8 timeout = 100;
    SCCB_SDA_IN();
    SCCB_SDA_H();
    sccb_delay();
    SCCB_SCL_H();
    sccb_delay();
    while(SCCB_SDA_READ()) {
        if(--timeout == 0) {
            sccb_stop();
            return 1;
        }
    }
    SCCB_SCL_L();
    SCCB_SDA_OUT();
    return 0;
}

static void sccb_send_noack(void) {
    SCCB_SDA_IN();
    SCCB_SDA_H();
    sccb_delay();
    SCCB_SCL_H();
    sccb_delay();
    SCCB_SCL_L();
    SCCB_SDA_OUT();
    SCCB_SDA_L();
}

static void sccb_send_ack(void) {
    SCCB_SDA_OUT();
    SCCB_SDA_L();
    sccb_delay();
    SCCB_SCL_H();
    sccb_delay();
    SCCB_SCL_L();
    SCCB_SDA_H();
}

static void sccb_write_byte(u8 data) {
    u8 i;
    SCCB_SDA_OUT();
    for(i = 0; i < 8; i++) {
        if(data & 0x80) SCCB_SDA_H();
        else            SCCB_SDA_L();
        data <<= 1;
        sccb_delay();
        SCCB_SCL_H();
        sccb_delay();
        SCCB_SCL_L();
    }
}

static u8 sccb_read_byte(void) {
    u8 i, data = 0;
    SCCB_SDA_IN();
    for(i = 0; i < 8; i++) {
        data <<= 1;
        SCCB_SCL_H();
        sccb_delay();
        if(SCCB_SDA_READ()) data |= 1;
        SCCB_SCL_L();
        sccb_delay();
    }
    SCCB_SDA_OUT();
    return data;
}

/* ============================================================
 *  OV5640 寄存器读写
 * ============================================================ */

static u8 OV5640_WriteReg(u16 reg, u8 data) {
    sccb_start();
    sccb_write_byte(OV5640_ADDR_WRITE);
    if(sccb_wait_ack()) return 1;
    sccb_write_byte((u8)(reg >> 8));
    if(sccb_wait_ack()) return 1;
    sccb_write_byte((u8)(reg & 0xFF));
    if(sccb_wait_ack()) return 1;
    sccb_write_byte(data);
    if(sccb_wait_ack()) return 1;
    sccb_stop();
    return 0;
}

static u8 OV5640_ReadReg(u16 reg, u8 *data) {
    sccb_start();
    sccb_write_byte(OV5640_ADDR_WRITE);
    if(sccb_wait_ack()) { sccb_stop(); return 1; }
    sccb_write_byte((u8)(reg >> 8));
    if(sccb_wait_ack()) { sccb_stop(); return 1; }
    sccb_write_byte((u8)(reg & 0xFF));
    if(sccb_wait_ack()) { sccb_stop(); return 1; }
    sccb_stop();

    sccb_start();
    sccb_write_byte(OV5640_ADDR_READ);
    if(sccb_wait_ack()) { sccb_stop(); return 1; }
    *data = sccb_read_byte();
    sccb_send_noack();
    sccb_stop();
    return 0;
}

/* ============================================================
 *  OV5640 寄存器配置表
 * ============================================================ */

/* 通用初始化 (上电 → 配置PLL → 设置输出格式) */
static const u16 ov5640_init_regs[][2] = {
    /* 软件复位 */
    {0x3008, 0x82},  /* 复位所有寄存器 */
    /* 等待10ms… 实际在调用侧delay */

    /* 系统时钟 */
    {0x3103, 0x11},  /* 系统时钟来自PLL */
    {0x3008, 0x42},  /* 停止睡眠 */

    /* IO控制 */
    {0x3017, 0x7F},  /* IO驱动强度 */
    {0x3018, 0xFC},  /* IO驱动强度 */

    /* PAD输出 */
    {0x3000, 0x00},  /* VSYNC/HSYNC */
    {0x3001, 0x00},
    {0x3002, 0x00},
    {0x3004, 0xFF},
    {0x3005, 0xFF},
    {0x3006, 0x43},
    {0x3007, 0x37},
    {0x3011, 0x08},

    /* 时钟设置 */
    {0x3010, 0x08},

    {0x460C, 0x22},  /* 8-bit模式 */
    {0x4815, 0x00},
    {0x4713, 0x02},
    {0x4514, 0x00},
    {0x4837, 0x0C},
    {0x3824, 0x01},  /* PCLK反相 */

    /* AEC/AGC */
    {0x3A0F, 0x30},
    {0x3A10, 0x28},
    {0x3A1B, 0x30},
    {0x3A1E, 0x26},
    {0x3A11, 0x60},
    {0x3A1F, 0x14},

    /* 50/60Hz */
    {0x3A00, 0x58},  /* 50Hz */

    /* 输出格式 RGB565 */
    {0x4300, 0x61},  /* RGB565 */
    {0x501F, 0x01},  /* ISP RGB */
};

/* QQVGA (160x120) 输出配置 */
static const u16 ov5640_qqvga_regs[][2] = {
    {0x3800, 0x00},  /* X起始高字节 */
    {0x3801, 0x00},  /* X起始低字节 */
    {0x3802, 0x00},  /* Y起始高字节 */
    {0x3803, 0x00},  /* Y起始低字节 */
    {0x3804, 0x0A},  /* X结束高字节 2623 */
    {0x3805, 0x3F},  /* X结束低字节 */
    {0x3806, 0x07},  /* Y结束高字节 1951 */
    {0x3807, 0x9F},  /* Y结束低字节 */
    {0x3808, 0x00},  /* 输出宽度高 160 */
    {0x3809, 0xA0},
    {0x380A, 0x00},  /* 输出高度高 120 */
    {0x380B, 0x78},
    {0x380C, 0x07},  /* HTS 高字节 */
    {0x380D, 0x68},  /* HTS 低字节 */
    {0x380E, 0x03},  /* VTS 高字节 */
    {0x380F, 0xD8},  /* VTS 低字节 */
    {0x3810, 0x00},  /* ISP X偏移 */
    {0x3811, 0x10},
    {0x3812, 0x00},  /* ISP Y偏移 */
    {0x3813, 0x06},
    {0x3814, 0x31},  /* X inc */
    {0x3815, 0x31},  /* Y inc */
};

/* QVGA (320x240) 输出配置 */
static const u16 ov5640_qvga_regs[][2] = {
    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x00},
    {0x3804, 0x0A},
    {0x3805, 0x3F},
    {0x3806, 0x07},
    {0x3807, 0x9F},
    {0x3808, 0x01},  /* 宽度 320 */
    {0x3809, 0x40},
    {0x380A, 0x00},  /* 高度 240 */
    {0x380B, 0xF0},
    {0x380C, 0x07},
    {0x380D, 0x68},
    {0x380E, 0x03},
    {0x380F, 0xD8},
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x06},
    {0x3814, 0x31},
    {0x3815, 0x31},
};

/* ============================================================
 *  批量写寄存器
 * ============================================================ */

static void OV5640_WriteRegs(const u16 regs[][2], u16 count) {
    u16 i;
    for(i = 0; i < count; i++) {
        OV5640_WriteReg(regs[i][0], (u8)regs[i][1]);
    }
}

/* ============================================================
 *  SCCB / 控制 GPIO 初始化
 * ============================================================ */

static void OV5640_GPIO_Init(void) {
    GPIO_InitTypeDef gpio;
    RCC_AHB1PeriphClockCmd(OV_SCCB_RCC | OV_RST_RCC, ENABLE);

    /* SCCB: PB10=SCL, PB11=SDA (开漏输出) */
    gpio.GPIO_Pin   = OV_SCCB_SCL_PIN | OV_SCCB_SDA_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(OV_SCCB_PORT, &gpio);
    SCCB_SCL_H();
    SCCB_SDA_H();

    /* RST: PG15 (推挽) */
    gpio.GPIO_Pin   = OV_RST_PIN;
    gpio.GPIO_Mode  = GPIO_Mode_OUT;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(OV_RST_PORT, &gpio);
    GPIO_SetBits(OV_RST_PORT, OV_RST_PIN);
}

/* ============================================================
 *  XCLK 输出 (PA8 MCO1)
 * ============================================================ */

static void OV5640_XCLK_Init(void) {
    GPIO_InitTypeDef gpio;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* PA8 → MCO1 (AF0) */
    gpio.GPIO_Pin   = GPIO_Pin_8;
    gpio.GPIO_Mode  = GPIO_Mode_AF;
    gpio.GPIO_OType = GPIO_OType_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_MCO);

    /* MCO1 = HSE (8MHz) / 1 = 8MHz */
    RCC_MCO1Config(RCC_MCO1Source_HSE, RCC_MCO1Div_1);
}

/* ============================================================
 *  硬件复位
 * ============================================================ */

static void OV5640_HWReset(void) {
    GPIO_ResetBits(OV_RST_PORT, OV_RST_PIN);
    delay_ms(10);
    GPIO_SetBits(OV_RST_PORT, OV_RST_PIN);
    delay_ms(20);
}

/* ============================================================
 *  读取ID
 * ============================================================ */

u8 OV5640_ReadID(void) {
    u8 h, l;
    OV5640_ReadReg(0x300A, &h);
    OV5640_ReadReg(0x300B, &l);
    return ((u16)h << 8) | l;
}

/* ============================================================
 *  初始化
 * ============================================================ */

u8 OV5640_Init(void) {
    u16 id;

    /* GPIO / XCLK */
    OV5640_GPIO_Init();
    OV5640_XCLK_Init();
    OV5640_HWReset();

    /* 读ID验证 */
    id = OV5640_ReadID();
    if(id != OV5640_PID) return 1;

    /* 写通用初始化 */
    OV5640_WriteRegs(ov5640_init_regs, sizeof(ov5640_init_regs) / 4);
    delay_ms(50);

    /* 默认 QQVGA 模式 */
    OV5640_SetMode(OV5640_MODE_QQVGA_160x120);

    return 0;
}

/* ============================================================
 *  设置分辨率
 * ============================================================ */

void OV5640_SetMode(ov5640_mode_t mode) {
    switch(mode) {
    case OV5640_MODE_QQVGA_160x120:
        OV5640_WriteRegs(ov5640_qqvga_regs, sizeof(ov5640_qqvga_regs) / 4);
        break;
    case OV5640_MODE_QVGA_320x240:
        OV5640_WriteRegs(ov5640_qvga_regs, sizeof(ov5640_qvga_regs) / 4);
        break;
    default:
        break;
    }
    delay_ms(10);
}

/* ============================================================
 *  图像调节函数
 * ============================================================ */

void OV5640_SetLightMode(u8 mode) {
    u8 val;
    switch(mode) {
    case 0: OV5640_WriteReg(0x3212, 0x03); break; /* 自动 */
    case 1: OV5640_WriteReg(0x3212, 0x13); break; /* 晴天 */
    case 2: OV5640_WriteReg(0x3212, 0x23); break; /* 阴天 */
    case 3: OV5640_WriteReg(0x3212, 0x33); break; /* 办公室 */
    default: break;
    }
}

void OV5640_ColorSaturation(u8 sat) {
    OV5640_WriteReg(0x5001, sat & 0x7F);
    OV5640_WriteReg(0x5583, 0x10 | (sat > 0x40 ? 0x08 : 0));
    OV5640_WriteReg(0x5584, sat > 0x40 ? (sat - 0x40) : sat);
}

void OV5640_Brightness(u8 bright) {
    OV5640_WriteReg(0x5002, bright & 0x7F);
    OV5640_WriteReg(0x5589, 0x10 | (bright > 0x40 ? 0x08 : 0));
    OV5640_WriteReg(0x5580, bright > 0x40 ? (bright - 0x40) : bright);
}

void OV5640_Contrast(u8 contrast) {
    OV5640_WriteReg(0x5003, contrast & 0x7F);
    OV5640_WriteReg(0x5585, 0x10);
    OV5640_WriteReg(0x5586, contrast);
}

void OV5640_Sharpness(u8 sharp) {
    OV5640_WriteReg(0x5006, sharp & 0x7F);
}

void OV5640_Flip(u8 flip) {
    u8 val = 0;
    if(flip & 0x01) val |= 0x20;  /* 上下 */
    if(flip & 0x02) val |= 0x40;  /* 左右 */
    OV5640_WriteReg(0x3820, val);
    OV5640_WriteReg(0x3821, val);
}
