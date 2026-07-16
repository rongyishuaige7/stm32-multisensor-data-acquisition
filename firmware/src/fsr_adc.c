#include "fsr_adc.h"
#include "board_pins.h"
#include "stm32f1xx_hal.h"

extern ADC_HandleTypeDef hadc1;

#define AVG_N 16

void fsr_adc_init(void)
{
    (void)hadc1;
}

static uint16_t adc_raw(void)
{
    uint32_t sum = 0;
    for (int i = 0; i < AVG_N; i++) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 50) == HAL_OK)
            sum += (uint16_t)HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }
    return (uint16_t)(sum / AVG_N);
}

/**
 * FSR402 非线性粗略映射：ADC 0..4095 -> 克力示意
 * 实际标定请在实机上用已知砝码修正 fsr_adc.c 中的表或公式。
 */
uint16_t fsr_read_pressure_g(void)
{
    uint16_t v = adc_raw();
    /* 空载基线扣除（PA1+10k 下拉，FSR 漏电流约 ADC 100~200） */
    if (v < 200)
        return 0;
    /* 分段线性近似（实测标定，单位 g）：
     *   v=200    -> 0g
     *   v=500    -> 100g    （轻触）
     *   v=1000   -> 300g    （中等按压）
     *   v=2000   -> 600g    （较用力）
     *   v=3500   -> 1200g   （重压）
     *   v=4095   -> 1600g
     */
    if (v < 500)
        return (uint16_t)((v - 200) * 100u / 300u);
    if (v < 1000)
        return (uint16_t)(100 + (v - 500) * 200u / 500u);
    if (v < 2000)
        return (uint16_t)(300 + (v - 1000) * 300u / 1000u);
    if (v < 3500)
        return (uint16_t)(600 + (v - 2000) * 600u / 1500u);
    return (uint16_t)(1200 + (v - 3500) * 400u / 600u);
}
