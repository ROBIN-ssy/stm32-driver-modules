# STM32 自写驱动模块合集

平时攒下来的外设驱动，按 **F1（STM32F103）** 和 **F4（STM32F407）** 两套整理。
每个模块都是独立的 `.c/.h`，直接挂进工程即可，不依赖特定框架。

---

## 目录

```
stm32-driver-modules/
├── F1/                     STM32F103（标准库，Keil）
│   ├── scale/              电子秤
│   │   ├── AD.c/.h         内部 ADC 采样
│   │   ├── Key.c/.h        按键
│   │   ├── LED.c/.h        指示灯
│   │   └── OLED.c/.h + OLED_Font.h   0.96" OLED 显示
│   └── pressure/           压力传感器
│       ├── HX711.c/.h      24 位称重 ADC（HX711）
│       ├── Motor.c/.h      电机
│       ├── PWM.c/.h        PWM 输出
│       └── OLED.c/.h + OLED_Font.h
└── F4/                     STM32F407（SPL，Keil）
    ├── tft/                2.4" TFT 显示
    │   ├── lcd.c/.h        基础绘图
    │   ├── lcdfont.h       字库
    │   ├── lcd_init.c/.h   面板初始化序列
    │   ├── tft.c/.h        上层显示
    │   ├── tft_spi.c/.h    SPI 底层
    │   ├── zk.c            中文字库读取
    │   └── pic.h           图片数据
    ├── camera/             ★ OV5640 摄像头 → TFT 实时显示
    │   ├── dcmi.c/.h       DCMI 并口采集
    │   ├── ov5640.c/.h     SCCB(I²C) 配置
    │   ├── lcd.c/.h        显示
    │   ├── main.c          完整示例
    │   └── README.md       ★ 性能优化记录
    └── ble-wave/           蓝牙传波形
        └── wave_plotter.c/.h   采样数据打包成上位机可绘制的格式
```

---

## 最值得看的一个：OV5640 + TFT 从 3 fps 到 25 fps

`F4/camera/README.md` 记录了整个过程，结论：

| 方案 | SPI 速率 | 帧率 |
|---|---|---|
| 软件 SPI（初版） | ~4 Mbps | ~3 fps |
| **硬件 SPI2 + DMA** | 10.5 MHz | **~25 fps** |

关键动作是把逐字节的软件 SPI 换掉，让 DMA 一口气搬 38400 字节：

```
摄像头 (DCMI) ──→ cam_frame_buf [38400 B] ──DMA──→ TFT (SPI2)
                          CPU 在这段时间完全空闲
```

**这不是"换成 DMA 就好了"那么简单**——真正花时间的是：SPI 时序参数、DMA 搬运宽度、
帧缓冲大小与 TFT 窗口设置之间的对齐。细节都在那份 README 里。

---

## 其他模块

**`F1/pressure/HX711.c`** — 24 位称重 ADC 的位操作时序。
HX711 是典型的两线（SCK+DOUT）器件，靠时钟数量选择通道与增益，值得一读。

**`F4/ble-wave/wave_plotter.c`** — 把 ADC 采样序列打包成上位机能直接画的格式，
经蓝牙发出去。解决的是"怎么在手机/电脑上看到波形"这个问题。

**`F4/tft/`** — 完整的一套 TFT 驱动分层：`tft_spi.c`（时序）→ `lcd.c`（绘图）→ `tft.c`（应用）。

---

## 说明

- **F1 与 F4 是两套不同的库**：F1 用 ST 标准外设库（SPL）风格，F4 也是 SPL，
  但两代 SPL API 不同，所以分开放在两个目录，不能混用
- `F1/scale` 和 `F1/pressure` 各有一份 `OLED.c/h` + `OLED_Font.h`，内容基本一致，
  **这是原样保留的**——两个工程当时各自独立开发。想复用的话取其一即可
- 仓库只放驱动源码，不含 ST 官方库（`FWLIB`）和构建产物（`OBJ`）。
  挂进工程前需要你自己补 STM32 标准外设库和 Keil 工程

---

## 许可

MIT。字库、图片数据（`lcdfont.h`、`pic.h`、`OLED_Font.h`）如来自第三方，版权归原作者。
