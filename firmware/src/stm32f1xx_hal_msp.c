#include "stm32f1xx_hal.h"
#include "board_pins.h"

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef hadc1;

void HAL_MspInit(void)
{
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitTypeDef g = {0};
        g.Mode = GPIO_MODE_AF_PP;
        g.Speed = GPIO_SPEED_FREQ_HIGH;
        g.Pull = GPIO_NOPULL;
        g.Pin = GPIO_PIN_5 | GPIO_PIN_7;
        HAL_GPIO_Init(GPIOA, &g);
        g.Pin = GPIO_PIN_6;
        g.Mode = GPIO_MODE_INPUT;
        g.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &g);

        g.Mode = GPIO_MODE_OUTPUT_PP;
        g.Speed = GPIO_SPEED_FREQ_HIGH;
        g.Pin = W25Q64_CS_Pin;
        HAL_GPIO_Init(W25Q64_CS_GPIO_Port, &g);
        HAL_GPIO_WritePin(W25Q64_CS_GPIO_Port, W25Q64_CS_Pin, GPIO_PIN_SET);
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef g = {0};
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        g.Pin = GPIO_PIN_9;
        g.Mode = GPIO_MODE_AF_PP;
        g.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &g);
        g.Pin = GPIO_PIN_10;
        g.Mode = GPIO_MODE_INPUT;
        g.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOA, &g);

        HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    } else if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        g.Pin = GPIO_PIN_2;
        g.Mode = GPIO_MODE_AF_PP;
        g.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &g);
        g.Pin = GPIO_PIN_3;
        g.Mode = GPIO_MODE_INPUT;
        g.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOA, &g);

        HAL_NVIC_SetPriority(USART2_IRQn, 1, 1);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) {
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitTypeDef g = {0};
        g.Pin = FSR_Pin;
        g.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(FSR_GPIO_Port, &g);
    }
}
