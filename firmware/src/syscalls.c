#include "stm32f1xx_hal.h"
#include "board_pins.h"
#include <errno.h>
#include <sys/stat.h>

extern UART_HandleTypeDef huart2;

int _write(int file, char *ptr, int len)
{
    (void)file;
    if (HAL_UART_Transmit(&huart2, (uint8_t *)ptr, (uint16_t)len, 100) != HAL_OK)
        return 0;
    return len;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

void _exit(int status)
{
    (void)status;
    while (1) {
    }
}
