#include "photo_e3z.h"
#include "board_pins.h"
#include "stm32f1xx_hal.h"

static uint8_t s_prev_raw;
static uint8_t s_have_prev;
static uint8_t s_out;

void photo_e3z_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = PHOTO_Pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(PHOTO_GPIO_Port, &g);
    s_have_prev = 0;
    s_prev_raw = 0;
    s_out = 0;
}

/**
 * 非阻塞消抖：连续两个 1Hz tick 读到的原始电平一致则更新输出；
 * 上电第一次直接采信当前电平。
 */
uint8_t photo_e3z_read_stable(void)
{
    GPIO_PinState pin = HAL_GPIO_ReadPin(PHOTO_GPIO_Port, PHOTO_Pin);
    uint8_t raw = (uint8_t)((pin == GPIO_PIN_SET) ? 1 : 0);

    if (!s_have_prev) {
        s_have_prev = 1;
        s_prev_raw = raw;
        s_out = raw;
        return s_out;
    }
    if (raw == s_prev_raw)
        s_out = raw;
    s_prev_raw = raw;
    return s_out;
}
