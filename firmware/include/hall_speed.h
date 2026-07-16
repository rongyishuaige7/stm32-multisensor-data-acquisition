#ifndef HALL_SPEED_H
#define HALL_SPEED_H

#include <stdint.h>

void hall_speed_init(void);
uint16_t hall_speed_rpm_tick_1hz(void);

#endif
