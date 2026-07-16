#include "alarm.h"
#include "board_pins.h"
#include "stm32f1xx_hal.h"

/* 硬编码阈值 */
#define TH_TEMP_C 27.0f
#define TH_RPM 59U
#define TH_PRESS_G 100U

static uint8_t s_prev_light_valid;
static uint8_t s_prev_light;
static uint8_t s_manual_on;     /* 按键/远程手动开关：0 = 关，1 = 强制响 */
static uint8_t s_auto_on;       /* 报警自动置位：0 = 无报警，1 = 有报警 */
static uint8_t s_key_stable;    /* 去抖后按键电平：0=释放(高)，1=按下(低) */
static uint8_t s_key_cnt;       /* 连续采样同态计数 */

static void buzzer_apply(void)
{
    /* 蜂鸣器低电平有效（NPN 三极管基极）：RESET=响，SET=关
     * 手动 OR 自动任一置位即响 */
    uint8_t on = s_manual_on | s_auto_on;
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin,
                      on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void alarm_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin = BUZZER_Pin;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_GPIO_Port, &g);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);

    GPIO_InitTypeDef k = {0};
    k.Pin = KEY_MUTE_Pin;
    k.Mode = GPIO_MODE_INPUT;
    k.Pull = GPIO_PULLUP;
    k.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY_MUTE_GPIO_Port, &k);

    s_prev_light_valid = 0;
    s_prev_light = 0;
    s_manual_on = 0;
    s_auto_on = 0;
    s_key_stable = 0;
    s_key_cnt = 0;
    buzzer_apply();
}

uint8_t alarm_buzzer_on(void) { return (s_manual_on | s_auto_on) ? 1 : 0; }

void alarm_buzzer_set(uint8_t on)
{
    s_manual_on = on ? 1 : 0;
    buzzer_apply();
}

uint8_t alarm_key_poll(void)
{
    uint8_t raw_pressed =
        (HAL_GPIO_ReadPin(KEY_MUTE_GPIO_Port, KEY_MUTE_Pin) == KEY_MUTE_ACTIVE_LEVEL) ? 1 : 0;

    if (raw_pressed == s_key_stable) {
        s_key_cnt = 0;
        return 0;
    }
    if (++s_key_cnt < 3)
        return 0;

    s_key_cnt = 0;
    s_key_stable = raw_pressed;

    /* 下降沿（释放→按下）瞬间翻转蜂鸣器开/关 */
    if (s_key_stable) {
        s_manual_on ^= 1u;
        buzzer_apply();
        return 1;
    }
    return 0;
}

uint8_t alarm_compute(float temp_c, uint16_t rpm, uint16_t pressure_g, uint8_t light_on)
{
    uint8_t m = 0;
    if (temp_c > TH_TEMP_C)
        m |= ALARM_TEMP;
    if (rpm > TH_RPM)
        m |= ALARM_RPM;
    if (pressure_g > TH_PRESS_G)
        m |= ALARM_PRESS;
    if (s_prev_light_valid && (light_on != s_prev_light))
        m |= ALARM_LIGHT;
    s_prev_light_valid = 1;
    s_prev_light = light_on;
    return m;
}

void alarm_set_buzzer(uint8_t alarm_mask)
{
    /* 温度/转速/压力触发蜂鸣器；光照变化仅作为事件上报，不驱动蜂鸣器 */
    s_auto_on = (alarm_mask & (ALARM_TEMP | ALARM_RPM | ALARM_PRESS)) ? 1 : 0;
    buzzer_apply();
}
