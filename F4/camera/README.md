# OV5640 → 2.4寸 TFT 摄像头实时显示 (SPI2+DMA版)

## 性能对比

| 方案 | 速度 | 帧率 |
|------|------|:---:|
| 软件SPI (旧) | ~4 Mbps | ~3 fps |
| **硬件SPI2+DMA (新)** | 10.5 MHz | **~25 fps** |

DMA 一次性轰 38400 字节，CPU 在此期间完全空闲。

## 原理

```
摄像头 (DCMI) ──→ cam_frame_buf [38400B]
                       │  memcpy (~2ms)
                       ▼
                  disp_buf [38400B]
                       │  SPI2 DMA (~28ms)
                       ▼
                  ILI9341 TFT
```

双缓冲 + memcpy 防止 DCMI 和 SPI DMA 同时访问同一块内存导致撕裂。

## SPI2 管脚 (AF5)

| TFT | STM32 | 模式 |
|-----|-------|------|
| SDA | PB15  | SPI2_MOSI (AF5) |
| SCL | PB13  | SPI2_SCK (AF5) |
| CS  | PB12  | GPIO (手动控) |
| RS  | PC5   | GPIO (手动控) |
| RST | PB14  | GPIO (推挽) |
| BLK | PB1   | GPIO |

SPI2 配置为 **TX-only 模式**，PB14 的 MISO 功能被释放出来，仍作 RST 用。

## DMA

- **DMA1_Stream4_Channel0** → SPI2_TX
- 单次传输模式，每帧触发一次

## 文件清单

| 文件 | 说明 |
|------|------|
| `ov5640.c/h` | OV5640 驱动 |
| `dcmi.c/h`   | DCMI+DMA 采集 (DMA2_Stream1) |
| `lcd.c/h`    | ILI9341 驱动 (硬件SPI2 + DMA1_Stream4) |
| `main.c`     | 主程序 |
