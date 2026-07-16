#include "hall_speed.h"
#include "board_pins.h"
#include "stm32f1xx_hal.h"

static volatile uint32_t s_pulse_count;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == HALL_Pin)
        s_pulse_count++;
}

void hall_speed_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin = HALL_Pin;
    g.Mode = GPIO_MODE_IT_FALLING;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(HALL_GPIO_Port, &g);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
    /* 清除可能的伪触发（GPIO 切换瞬间产生的 pending） */
    __HAL_GPIO_EXTI_CLEAR_IT(HALL_Pin);
    s_pulse_count = 0;
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/** 每秒调用一次：假设每转 1 个脉冲 */
uint16_t hall_speed_rpm_tick_1hz(void)
{
    uint32_t c;
    __disable_irq();
    c = s_pulse_count;
    s_pulse_count = 0;
    __enable_irq();
    if (c > 0xFFFFu / 60u)
        c = 0xFFFFu / 60u;
    return (uint16_t)(c * 60u);
}
