#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>
#include "app_types.h"

void alarm_init(void);
uint8_t alarm_compute(float temp_c, uint16_t rpm, uint16_t pressure_g, uint8_t light_on);

/* 蜂鸣器由按键手动控制（不再跟随报警）。alarm_mask 仍作为数据字段上报。
 * 此函数保留以兼容旧调用，内部只刷新当前手动开/关状态到 GPIO。 */
void alarm_set_buzzer(uint8_t alarm_mask);

/* 按键（PB5）轮询：需在主循环快速路径每轮调用一次，含软件去抖。
 * 检测到单击下降沿即翻转蜂鸣器开/关。返回 1 表示本次调用发生了切换。 */
uint8_t alarm_key_poll(void);

/* 查询/设置蜂鸣器手动开关状态（1 = 强制响，0 = 关）。 */
uint8_t alarm_buzzer_on(void);
void    alarm_buzzer_set(uint8_t on);

#endif
