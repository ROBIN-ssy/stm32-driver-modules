#ifndef __WAVE_PLOTTER_H
#define __WAVE_PLOTTER_H

#include "stdint.h"
#include "main.h"   // 将原来的 "usart.h" 替换为 "main.h" 即可完美解决

/*
 * @brief  发送单通道波形数组 (含电压映射)
 * @param  huart  目标串口句柄的指针 (例如 &huart2)
 * @param  data   数据数组指针
 * @param  length 数组的长度
 */
void Plotter_Send_1CH(UART_HandleTypeDef *huart, int *data, uint16_t length);

/*
 * @brief  发送双通道波形数组 (含电压映射)
 * @param  huart  目标串口句柄的指针 (例如 &huart2)
 * @param  data1  通道1数据数组指针
 * @param  data2  通道2数据数组指针
 * @param  length 数组的长度
 */
void Plotter_Send_2CH(UART_HandleTypeDef *huart, int *data1, int *data2, uint16_t length);

#endif /* __WAVE_PLOTTER_H */
