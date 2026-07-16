#include "stm32f1xx_it.h"
#include "stm32f1xx_hal.h"
#include "board_pins.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

volatile uint32_t g_hardfault_sp;
volatile uint32_t g_hardfault_pc;
volatile uint32_t g_hardfault_lr;

void NMI_Handler(void)
{
    while (1) {
    }
}

void HardFault_C(uint32_t *sp)
{
    g_hardfault_sp = (uint32_t)sp;
    g_hardfault_pc = sp[6];
    g_hardfault_lr = sp[5];
    __disable_irq();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = LED_Pin;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &g);
    while (1) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        for (volatile uint32_t i = 0; i < 400000U; i++) {
        }
    }
}

__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4            \n"
        "ite eq                \n"
        "mrseq r0, msp         \n"
        "mrsne r0, psp         \n"
        "ldr r1, =HardFault_C  \n"
        "bx r1                 \n");
}

void MemManage_Handler(void)
{
    while (1) {
    }
}

void BusFault_Handler(void)
{
    while (1) {
    }
}

void UsageFault_Handler(void)
{
    while (1) {
    }
}

void DebugMon_Handler(void)
{
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
