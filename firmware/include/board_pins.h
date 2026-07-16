/**
 * @file board_pins.h
 * @brief 引脚分配（与方案一致；W25Q64 CS 改为 PB1，避免与 USART2_RX/PA3 冲突）
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32f1xx_hal.h"

/* DS18B20 — 1-Wire */
#define DS18B20_GPIO_Port GPIOB
#define DS18B20_Pin GPIO_PIN_12

/* A1104 霍尔 — TIM2_CH1 输入捕获 */
#define HALL_GPIO_Port GPIOA
#define HALL_Pin GPIO_PIN_0

/* FSR402 — ADC1_IN1 */
#define FSR_GPIO_Port GPIOA
#define FSR_Pin GPIO_PIN_1

/* E3Z-LS63 — 数字输入 */
#define PHOTO_GPIO_Port GPIOA
#define PHOTO_Pin GPIO_PIN_4

/* 蜂鸣器 */
#define BUZZER_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_0

/* 静音按键 — PB5，低有效（按下接 GND），内部上拉 */
#define KEY_MUTE_GPIO_Port GPIOB
#define KEY_MUTE_Pin GPIO_PIN_5
#define KEY_MUTE_ACTIVE_LEVEL GPIO_PIN_RESET

/* W25Q64 — SPI1，CS 使用 PB1（方案中 PA3 与 ESP RX 冲突） */
#define W25Q64_SPI hspi1
#define W25Q64_CS_GPIO_Port GPIOB
#define W25Q64_CS_Pin GPIO_PIN_1

/* ESP-01S — USART1: PA9 TX, PA10 RX */
#define ESP_UART huart1

/* 调试串口 — USART2: PA2 TX, PA3 RX */
#define DEBUG_UART huart2

/* Blue Pill 板载 LED（PC13，低电平亮） */
#define LED_GPIO_Port GPIOC
#define LED_Pin GPIO_PIN_13

/* 0.96" OLED (SSD1306, I2C, 软件位拨) */
#define OLED_SCL_GPIO_Port GPIOB
#define OLED_SCL_Pin GPIO_PIN_6
#define OLED_SDA_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_7
#define OLED_I2C_ADDR 0x3C

#endif /* BOARD_PINS_H */
