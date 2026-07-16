#ifndef W25Q64_H
#define W25Q64_H

#include <stdint.h>
#include <stdbool.h>

#define W25Q64_FLASH_SIZE (0x800000UL)

void w25q64_init(void);
bool w25q64_read_id(uint32_t *jedec24);
void w25q64_read(uint32_t addr, uint8_t *buf, uint32_t len);
bool w25q64_wait_busy(uint32_t timeout_ms);
bool w25q64_write_enable(void);
bool w25q64_sector_erase_4k(uint32_t addr);
bool w25q64_page_program(uint32_t addr, const uint8_t *data, uint16_t len);

#endif
