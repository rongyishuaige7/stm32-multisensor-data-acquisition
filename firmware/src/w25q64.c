#include "w25q64.h"
#include "board_pins.h"
#include "stm32f1xx_hal.h"

extern SPI_HandleTypeDef hspi1;

static inline void cs_low(void)
{
    HAL_GPIO_WritePin(W25Q64_CS_GPIO_Port, W25Q64_CS_Pin, GPIO_PIN_RESET);
}

static inline void cs_high(void)
{
    HAL_GPIO_WritePin(W25Q64_CS_GPIO_Port, W25Q64_CS_Pin, GPIO_PIN_SET);
}

static void spi_tx(const uint8_t *d, uint16_t n)
{
    HAL_SPI_Transmit(&hspi1, (uint8_t *)d, n, 1000);
}

static void spi_rx(uint8_t *d, uint16_t n)
{
    HAL_SPI_Receive(&hspi1, d, n, 1000);
}

static void spi_txrx(const uint8_t *tx, uint8_t *rx, uint16_t n)
{
    HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)tx, rx, n, 1000);
}

void w25q64_init(void)
{
    cs_high();
}

bool w25q64_read_id(uint32_t *jedec24)
{
    if (!jedec24)
        return false;
    uint8_t cmd[4] = {0x9F, 0xFF, 0xFF, 0xFF};
    uint8_t rx[4];
    cs_low();
    spi_txrx(cmd, rx, 4);
    cs_high();
    *jedec24 = ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
    return rx[1] != 0 || rx[2] != 0;
}

void w25q64_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t hdr[4] = {0x03, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr};
    cs_low();
    spi_tx(hdr, 4);
    spi_rx(buf, (uint16_t)len);
    cs_high();
}

bool w25q64_wait_busy(uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    uint8_t st;
    do {
        uint8_t cmd = 0x05;
        cs_low();
        spi_tx(&cmd, 1);
        spi_rx(&st, 1);
        cs_high();
        if ((st & 1) == 0)
            return true;
    } while ((HAL_GetTick() - t0) < timeout_ms);
    return false;
}

bool w25q64_write_enable(void)
{
    uint8_t cmd = 0x06;
    cs_low();
    spi_tx(&cmd, 1);
    cs_high();
    return true;
}

bool w25q64_sector_erase_4k(uint32_t addr)
{
    if (!w25q64_write_enable())
        return false;
    uint8_t cmd[4] = {0x20, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr};
    cs_low();
    spi_tx(cmd, 4);
    cs_high();
    return w25q64_wait_busy(3000);
}

bool w25q64_page_program(uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (len == 0 || len > 256)
        return false;
    if (!w25q64_write_enable())
        return false;
    uint8_t cmd[4] = {0x02, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr};
    cs_low();
    spi_tx(cmd, 4);
    spi_tx(data, len);
    cs_high();
    return w25q64_wait_busy(100);
}
