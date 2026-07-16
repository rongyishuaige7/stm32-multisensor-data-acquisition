#ifndef ESP_UART_H
#define ESP_UART_H

#include <stdbool.h>
#include <stdint.h>
#include "app_types.h"

/* AT 驱动版 ESP-01S：STM32 通过 USART1（PA9/PA10）直接发 AT 指令，
 * 让 ESP-01S 连 WiFi 并开 TCP 多连接服务器，向所有客户端广播 JSON。
 * ESP-01S 应保持出厂/官方 AT 固件（推荐 1.7.x），不需要烧任何自定义代码。 */

void esp_uart_init(void);

/* 主循环周期调用：处理 ESP 串口收到的 URC（连接/断开/+IPD 数据等） */
void esp_uart_pump_rx(void);

/* AT 状态机推进：连接 WiFi、开 TCP 服务器；进入 ready 后才会 wifi_online()=true */
void esp_uart_tick(void);

bool esp_uart_wifi_online(void);

/** 获取最近一次 AT+CIFSR 解析到的本机 STA IP 字符串（如 "192.168.1.123"）。
 *  尚未拿到 IP 时返回空字符串。 */
const char *esp_uart_get_ip(void);

/** @return 至少一个已连接 link 发送成功（无连接时 false） */
bool esp_uart_send_data_json(const sensor_snapshot_t *s);
bool esp_uart_send_cached_record_json(const flash_record_t *r);

#endif
