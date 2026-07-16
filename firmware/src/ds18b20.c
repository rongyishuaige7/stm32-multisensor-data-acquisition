#include "ds18b20.h"
#include "board_pins.h"
#include "stm32f1xx_hal.h"

static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks) {
    }
}

static void pin_out(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin = DS18B20_Pin;
    g.Mode = GPIO_MODE_OUTPUT_OD;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_GPIO_Port, &g);
}

static void pin_release(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin = DS18B20_Pin;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DS18B20_GPIO_Port, &g);
}

static void write_bit(int b)
{
    pin_out();
    HAL_GPIO_WritePin(DS18B20_GPIO_Port, DS18B20_Pin, GPIO_PIN_RESET);
    delay_us(b ? 6 : 60);
    pin_release();
    delay_us(b ? 64 : 10);
}

static int read_bit(void)
{
    int v;
    pin_out();
    HAL_GPIO_WritePin(DS18B20_GPIO_Port, DS18B20_Pin, GPIO_PIN_RESET);
    delay_us(6);
    pin_release();
    delay_us(9);
    v = HAL_GPIO_ReadPin(DS18B20_GPIO_Port, DS18B20_Pin) ? 1 : 0;
    delay_us(55);
    return v;
}

static void write_byte(uint8_t x)
{
    for (int i = 0; i < 8; i++) {
        write_bit(x & 1);
        x >>= 1;
    }
}

static uint8_t read_byte(void)
{
    uint8_t x = 0;
    for (int i = 0; i < 8; i++) {
        x >>= 1;
        if (read_bit())
            x |= 0x80;
    }
    return x;
}

static bool reset_pulse(void)
{
    pin_out();
    HAL_GPIO_WritePin(DS18B20_GPIO_Port, DS18B20_Pin, GPIO_PIN_RESET);
    delay_us(480);
    pin_release();
    delay_us(70);
    int pres = HAL_GPIO_ReadPin(DS18B20_GPIO_Port, DS18B20_Pin) ? 0 : 1;
    delay_us(410);
    return pres != 0;
}

void ds18b20_init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    dwt_init();
    pin_release();
}

bool ds18b20_start_conversion(void)
{
    if (!reset_pulse())
        return false;
    write_byte(0xCC);
    write_byte(0x44);
    return true;
}

bool ds18b20_read_result_c(float *out_c)
{
    if (!out_c || !reset_pulse())
        return false;
    write_byte(0xCC);
    write_byte(0xBE);

    uint8_t lo = read_byte();
    uint8_t hi = read_byte();
    (void)reset_pulse();

    int16_t raw = (int16_t)((hi << 8) | lo);
    *out_c = (float)raw / 16.0f;
    return true;
}

bool ds18b20_read_temp_c(float *out_c)
{
    if (!ds18b20_start_conversion())
        return false;
    HAL_Delay(750);
    return ds18b20_read_result_c(out_c);
}
