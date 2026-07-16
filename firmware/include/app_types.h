#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/** 传感器快照（上位机 JSON 与 Flash 记录共用逻辑字段） */
typedef struct {
    float temp_c;       /**< DS18B20 摄氏度 */
    uint16_t rpm;       /**< 霍尔估算 RPM */
    uint16_t pressure_g;/**< FSR402 映射为克（示意） */
    uint8_t light_on;   /**< 光电：1=亮/通光，0=暗 */
    uint8_t alarm;      /**< 报警标志位掩码 */
    uint32_t uptime_s;  /**< 上电后秒计数 */
} sensor_snapshot_t;

/**
 * 紧凑 Flash 记录 12 字节：timestamp(4) + temp_x100(2) + rpm(2) + pressure(2) + light(1) + flags(1)
 * packed + aligned(1) 便于从 Flash 任意偏移安全 memcpy
 */
typedef struct __attribute__((packed, aligned(1))) {
    uint32_t timestamp_s;
    int16_t temp_c_x100;   /* 温度×100，整数 */
    uint16_t rpm;
    uint16_t pressure_g;
    uint8_t light_on;
    uint8_t flags;
} flash_record_t;

#define ALARM_TEMP   (1u << 0)
#define ALARM_RPM    (1u << 1)
#define ALARM_PRESS  (1u << 2)
#define ALARM_LIGHT  (1u << 3)

#endif
