#include "wave_plotter.h"
#include <stdio.h> // 提供 sprintf 支持
#include "main.h"  // 提供 HAL_Delay 支持

void Plotter_Send_1CH(UART_HandleTypeDef *huart, int *data, uint16_t length)
{
    char tx_buf[32]; // 定义一个小型字符数组作为发送缓存
    int mapped_val = 0;
    uint16_t str_len = 0;
    
    for (uint16_t i = 0; i < length; i++)
    {
        // 1. 电压映射 (0~4095 -> 0~330)
        mapped_val = (data[i] * 330) / 4095;
        
        // 2. 将数据组装成字符串存入 tx_buf，并获取字符串的真实长度
        str_len = sprintf(tx_buf, "[p,%d]", mapped_val);
        
        // 3. 直接调用 HAL 库串口发送该字符串
        HAL_UART_Transmit(huart, (uint8_t *)tx_buf, str_len, HAL_MAX_DELAY);
        
        // 4. 防拥堵小延时
        HAL_Delay(2);
    }
}

void Plotter_Send_2CH(UART_HandleTypeDef *huart, int *data1, int *data2, uint16_t length)
{
    char tx_buf[32];
    int mapped_val1 = 0;
    int mapped_val2 = 0;
    uint16_t str_len = 0;
    
    for (uint16_t i = 0; i < length; i++)
    {
        mapped_val1 = (data1[i] * 330) / 4095;
        mapped_val2 = (data2[i] * 330) / 4095;
        
        str_len = sprintf(tx_buf, "[p,%d,%d]", mapped_val1, mapped_val2);
        
        HAL_UART_Transmit(huart, (uint8_t *)tx_buf, str_len, HAL_MAX_DELAY);
        
        HAL_Delay(2);
    }
}
