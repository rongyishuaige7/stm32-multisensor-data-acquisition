#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include <stdbool.h>

void ds18b20_init(void);

/** 启动一次温度转换（非阻塞，约 750ms 后可用 read_result） */
bool ds18b20_start_conversion(void);

/** 读取上一轮转换结果（需在上次 start 后 ≥750ms 调用） */
bool ds18b20_read_result_c(float *out_c);

/** 单次阻塞读：start + 延时 + read（调试用） */
bool ds18b20_read_temp_c(float *out_c);

#endif
