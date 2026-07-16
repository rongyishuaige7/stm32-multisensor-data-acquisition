/**
 * @file oled.h
 * @brief 0.96" SSD1306 128x64 OLED 驱动（软件 I2C，PB6=SCL / PB7=SDA）
 */
#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include <stdbool.h>
#include "app_types.h"

void oled_init(void);
void oled_clear(void);
void oled_show(void);

/** 在第 page 行（0..7，每行 8px）从 x 起写一串 6x8 ASCII 字符 */
void oled_draw_str(uint8_t x, uint8_t page, const char *s);

/** 把传感器快照渲染到屏上（5 行） */
void oled_render_snapshot(const sensor_snapshot_t *snap, bool wifi_online);

#endif
