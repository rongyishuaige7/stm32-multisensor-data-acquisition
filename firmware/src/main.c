#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_iwdg.h"
#include "board_pins.h"
#include "app_types.h"
#include "ds18b20.h"
#include "fsr_adc.h"
#include "hall_speed.h"
#include "photo_e3z.h"
#include "w25q64.h"
#include "storage.h"
#include "alarm.h"
#include "esp_uart.h"
#include "oled.h"
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
ADC_HandleTypeDef hadc1;
IWDG_HandleTypeDef hiwdg;

void Error_Handler(void);
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_IWDG_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();

    w25q64_init();
    storage_init();
    ds18b20_init();
    fsr_adc_init();
    hall_speed_init();
    photo_e3z_init();
    alarm_init();
    esp_uart_init();
    oled_init();

    /* 首轮温度转换：下一秒 read_result 可取数 */
    (void)ds18b20_start_conversion();

    uint32_t jedec = 0;
    if (w25q64_read_id(&jedec))
        printf("W25Q JEDEC %06lX\r\n", (unsigned long)jedec);
    else
        printf("W25Q not detected\r\n");

    MX_IWDG_Init();

    uint32_t last_tick = HAL_GetTick();
    uint32_t uptime_s = 0;
    uint32_t last_meta = 0;

    while (1) {
        esp_uart_pump_rx();
        esp_uart_tick();

        if (alarm_key_poll())
            printf("BUZZ=%u\r\n", (unsigned)alarm_buzzer_on());

        uint32_t now = HAL_GetTick();
        uint32_t delta = now - last_tick;
        if (delta < 1000U)
            continue;

        if (delta >= 5000U) {
            last_tick = now;
            uptime_s++;
        } else {
            uint32_t n = delta / 1000U;
            uptime_s += n;
            last_tick += n * 1000U;
        }

        (void)HAL_IWDG_Refresh(&hiwdg);

        sensor_snapshot_t snap;
        memset(&snap, 0, sizeof(snap));
        snap.uptime_s = uptime_s;

        float tc = 0.0f;
        if (ds18b20_read_result_c(&tc))
            snap.temp_c = tc;
        else
            snap.temp_c = -999.0f;
        (void)ds18b20_start_conversion();

        snap.rpm = hall_speed_rpm_tick_1hz();
        snap.pressure_g = fsr_read_pressure_g();
        snap.light_on = photo_e3z_read_stable();

        if (snap.temp_c > -900.0f)
            snap.alarm = alarm_compute(snap.temp_c, snap.rpm, snap.pressure_g, snap.light_on);
        else
            snap.alarm = alarm_compute(0.0f, snap.rpm, snap.pressure_g, snap.light_on);

        alarm_set_buzzer(snap.alarm);

        bool wifi = esp_uart_wifi_online();
        storage_on_tick(&snap, wifi);
        oled_render_snapshot(&snap, wifi);

        if (wifi) {
            if (!esp_uart_send_data_json(&snap))
                (void)storage_ring_push_snapshot(&snap);
            flash_record_t cr;
            if (storage_ring_pop(&cr))
                (void)esp_uart_send_cached_record_json(&cr);
        }

        if (snap.temp_c < -900.0f) {
            printf("T=ERR  RPM=%u P=%u L=%u ALM=0x%02X WiFi=%d Up=%lus\r\n",
                   (unsigned)snap.rpm, (unsigned)snap.pressure_g,
                   (unsigned)snap.light_on, (unsigned)snap.alarm,
                   (int)wifi, (unsigned long)uptime_s);
        } else {
            int t_x100 = (int)(snap.temp_c * 100.0f);
            int t_int = t_x100 / 100;
            int t_frac = t_x100 % 100;
            if (t_frac < 0) t_frac = -t_frac;
            printf("T=%d.%02d RPM=%u P=%u L=%u ALM=0x%02X WiFi=%d Up=%lus\r\n",
                   t_int, t_frac, (unsigned)snap.rpm,
                   (unsigned)snap.pressure_g, (unsigned)snap.light_on,
                   (unsigned)snap.alarm, (int)wifi, (unsigned long)uptime_s);
        }

        if (uptime_s - last_meta >= 600U) {
            last_meta = uptime_s;
            storage_persist_meta();
        }

        (void)HAL_IWDG_Refresh(&hiwdg);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }
}

static void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload = 2500;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
        Error_Handler();
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        /* 无 8MHz 外部晶振时：HSI/2 × 16 ≈ 64MHz */
        RCC_OscInitTypeDef hsi = {0};
        hsi.OscillatorType = RCC_OSCILLATORTYPE_HSI;
        hsi.HSIState = RCC_HSI_ON;
        hsi.PLL.PLLState = RCC_PLL_ON;
        hsi.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
        hsi.PLL.PLLMUL = RCC_PLL_MUL16;
        if (HAL_RCC_OscConfig(&hsi) != HAL_OK)
            Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = LED_Pin;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_Port, &g);
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

static void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
        Error_Handler();
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
        Error_Handler();
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
        Error_Handler();
}

static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
        Error_Handler();
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
        Error_Handler();
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
        Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
