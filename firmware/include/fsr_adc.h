#ifndef FSR_ADC_H
#define FSR_ADC_H

#include <stdint.h>

void fsr_adc_init(void);
uint16_t fsr_read_pressure_g(void);

#endif
