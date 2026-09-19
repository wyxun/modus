/**
 * @file fixtures.h
 * @brief Test-owned register bindings for reusable provider templates.
 * @author Codex
 * @date 2026-09-18
 */
#ifndef MDI_CORE_TEST_FIXTURES_H
#define MDI_CORE_TEST_FIXTURES_H
#include "mdi/mdi.h"
#include "mdi/backend.h"
#include "mdi/pwm.h"

extern volatile uint32_t test_a_in;
extern volatile uint32_t test_a_out;
extern volatile uint32_t test_b_in;
extern volatile uint32_t test_b_out;
extern uint8_t test_eeprom_mem[256];
extern uint8_t test_eeprom_status;
extern uint32_t test_eeprom_write_count;
extern volatile uint32_t test_adc[4];
extern volatile uint16_t test_adc_dma[24];
extern volatile uint32_t test_adc_mean[3];
extern volatile uint32_t test_adc_mean_seq;
extern volatile uint32_t test_adc_published;
extern volatile uint32_t test_adc_consumed;
extern volatile bool test_adc_inject_publish;
extern volatile uint32_t test_adc_race_mean[3];
extern volatile uint32_t test_adc_race_mean_seq;
extern volatile bool test_adc_race_mean_valid;
extern volatile uint32_t test_adc_race_rate;
extern volatile uint16_t *test_adc_race_raw(void);
extern volatile uint32_t test_sample_seq;
extern volatile uint32_t test_sample_ready;
extern volatile uint32_t test_pwm_fault_latched;
extern volatile uint32_t test_pwm_fault_source;
extern volatile uint32_t test_ccr[4];
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t EGR;
    volatile uint32_t BDTR;
} test_timer_t;
extern test_timer_t test_timer;

/* Logical bit, physical bit, logical inversion. */
#define LED_PINS(X, V, ...) X(V, 0, 6, 1)
#define DATA_A_PINS(X, V, ...) X(V, 0, 1, 0) X(V, 1, 7, 0)
#define DATA_B_PINS(X, V, ...) X(V, 2, 2, 0) X(V, 3, 12, 0)
#define LED_PORTS(X, V) X(V, test_a_in, test_a_out, LED_PINS)
#define DATA_PORTS(X, V)                                                         \
    X(V, test_a_in, test_a_out, DATA_A_PINS)                                     \
    X(V, test_b_in, test_b_out, DATA_B_PINS)

MDI_STM32_IO_BIND(led, 1, LED_PORTS)
MDI_STM32_IO_BIND(data, 4, DATA_PORTS)

#define INPUT_PINS(X, V, ...) X(V, 0, 4, 0)
#define INPUT_PORTS(X, V) X(V, test_a_in, test_a_out, INPUT_PINS)
MDI_STM32_IO_BIND_INPUT_CAPS(input_only, 1, INPUT_PORTS, MDI_IO_CAP_INPUT)
#define OUTPUT_PINS(X, V, ...) X(V, 0, 5, 0)
#define OUTPUT_PORTS(X, V) X(V, test_a_in, test_a_out, OUTPUT_PINS)
MDI_STM32_IO_BIND_OUTPUT_CAPS(output_only, 1, OUTPUT_PORTS,
                              MDI_IO_CAP_OUTPUT)

#define SCL_PINS(X, V, ...) X(V, 0, 8, 0)
#define SDA_PINS(X, V, ...) X(V, 0, 7, 0)
#define SCL_PORTS(X, V) X(V, test_b_in, test_b_out, SCL_PINS)
#define SDA_PORTS(X, V) X(V, test_b_in, test_b_out, SDA_PINS)
MDI_STM32_IO_BIND_CAPS(sensor_scl, 1, SCL_PORTS,
                       MDI_IO_CAP_INPUT | MDI_IO_CAP_OUTPUT |
                       MDI_IO_CAP_OPEN_DRAIN)
MDI_STM32_IO_BIND_CAPS(sensor_sda, 1, SDA_PORTS,
                       MDI_IO_CAP_INPUT | MDI_IO_CAP_OUTPUT |
                       MDI_IO_CAP_OPEN_DRAIN)
MDI_SOFT_I2C_EDGES_BIND(sensor_bus, sensor_scl, sensor_sda)
MDI_INLINE void test_i2c_delay(void) { }
MDI_SOFT_I2C_MASTER_BIND(sensor_i2c, sensor_bus, test_i2c_delay, 5U, 8U)
MDI_I2C_REG8_BIND(sensor_reg, sensor_i2c, 0x36U)

#define EEPROM_CS_PINS(X, V, ...) X(V, 0, 3, 0)
#define EEPROM_CS_PORTS(X, V) X(V, test_a_in, test_a_out, EEPROM_CS_PINS)
MDI_STM32_IO_BIND(eeprom_cs, 1, EEPROM_CS_PORTS)

MDI_INLINE mdi_status_t eeprom_spi_spi_Transfer(
    const mdi_spi_transfer_t *ptTransfer)
{
    uint32_t wIndex;
    uint16_t hwAddress;
    if (ptTransfer == NULL || ptTransfer->pchTx == NULL ||
        ptTransfer->wLength == 0U) {
        return MDI_INVALID;
    }
    if (ptTransfer->pchRx != NULL) {
        for (wIndex = 0U; wIndex < ptTransfer->wLength; ++wIndex) {
            ptTransfer->pchRx[wIndex] = 0U;
        }
    }
    switch (ptTransfer->pchTx[0]) {
    case 0x06U:
        test_eeprom_status |= 0x02U;
        break;
    case 0x05U:
        if (ptTransfer->pchRx != NULL) {
            ptTransfer->pchRx[0] = test_eeprom_status;
        }
        break;
    case 0x02U:
        if (ptTransfer->wLength < 3U ||
            (test_eeprom_status & 0x02U) == 0U) {
            return MDI_IO_ERROR;
        }
        hwAddress = (uint16_t)(((uint16_t)ptTransfer->pchTx[1] << 8U) |
                               ptTransfer->pchTx[2]);
        for (wIndex = 3U; wIndex < ptTransfer->wLength; ++wIndex) {
            test_eeprom_mem[(hwAddress + wIndex - 3U) & 0xFFU] =
                ptTransfer->pchTx[wIndex];
        }
        test_eeprom_status &= (uint8_t)~0x02U;
        ++test_eeprom_write_count;
        break;
    case 0x03U:
        if (ptTransfer->wLength < 3U || ptTransfer->pchRx == NULL) {
            return MDI_INVALID;
        }
        hwAddress = (uint16_t)(((uint16_t)ptTransfer->pchTx[1] << 8U) |
                               ptTransfer->pchTx[2]);
        for (wIndex = 3U; wIndex < ptTransfer->wLength; ++wIndex) {
            ptTransfer->pchRx[wIndex] =
                test_eeprom_mem[(hwAddress + wIndex - 3U) & 0xFFU];
        }
        break;
    default:
        return MDI_IO_ERROR;
    }
    return MDI_OK;
}

MDI_SPI_EEPROM_BIND(eeprom, eeprom_spi, eeprom_cs, 256U, 16U, 8U)

/* Name, register lvalue, result mask, right shift. */
#define CURRENT_CHANNELS(X)                                                      \
    X(u, test_adc[0], 0xFFFFU, 0)                                                \
    X(v, test_adc[2], 0xFFFFU, 0)                                                \
    X(w, test_adc[1], 0xFFFFU, 0)
#define SENSOR_CHANNELS(X) X(value, test_adc[3], 0xFFU, 8)
MDI_SAMPLE_REG_BIND(current, CURRENT_CHANNELS)
MDI_SAMPLE_SEQ_BIND(stable_current, CURRENT_CHANNELS, test_sample_seq)
MDI_SAMPLE_READY_BIND(triggered_current, CURRENT_CHANNELS,
                      test_sample_ready, (test_sample_ready = 0U))
MDI_SAMPLE_REG_BIND(sensor, SENSOR_CHANNELS)
MDI_ADC_CHANNEL_BIND(raw_adc, test_adc[3], 0xFFU, 8)

#define ADC_MEAN_CHANNELS(X, ...)                                                \
    X(__VA_ARGS__, u, 0)                                                         \
    X(__VA_ARGS__, v, 1)                                                         \
    X(__VA_ARGS__, w, 2)
MDI_ADC_DMA_FLAG_BIND(test_adc_dma, test_adc_published, test_adc_consumed, 2U)
MDI_ADC_MEAN_BIND(test_adc_mean_filter, 4U, 3U, ADC_MEAN_CHANNELS,
                  test_adc_mean, test_adc_mean_seq)
#define RACE_ADC_RAW_CHANNELS(X, ...)                                            \
    X(__VA_ARGS__, u, 0)                                                         \
    X(__VA_ARGS__, v, 1)                                                         \
    X(__VA_ARGS__, w, 2)
MDI_ADC_MEAN_GROUP_BIND(test_adc_race_mean, test_adc_dma, test_adc_race_raw(),
                        4U, 3U, RACE_ADC_RAW_CHANNELS, test_adc_race_mean,
                        test_adc_race_mean_seq, test_adc_race_mean_valid,
                        test_adc_race_rate, 1000000U)
MDI_ADC_CHANNEL_SEQ_BIND(mean_u, test_adc_mean[0], 0xFFFFU, 0,
                         test_adc_mean_seq)
MDI_ADC_CHANNEL_SEQ_BIND(mean_v, test_adc_mean[1], 0xFFFFU, 0,
                         test_adc_mean_seq)
MDI_ADC_CHANNEL_SEQ_BIND(mean_w, test_adc_mean[2], 0xFFFFU, 0,
                         test_adc_mean_seq)

/* Name, compare register lvalue, inclusive maximum in timer ticks. */
#define BRIDGE_CHANNELS(X)                                                       \
    X(u, test_ccr[0], 4250U)                                                     \
    X(v, test_ccr[1], 4250U)                                                     \
    X(w, test_ccr[2], 4250U)
#define BUZZER_CHANNELS(X) X(value, test_ccr[3], 100U)
MDI_PWM_REG_BIND(bridge, BRIDGE_CHANNELS)
MDI_PWM_REG_BIND(buzzer, BUZZER_CHANNELS)

#define VARIABLE_CHANNELS(X)                                                     \
    X(u, test_ccr[0], test_timer.ARR)                                            \
    X(v, test_ccr[1], test_timer.ARR)                                            \
    X(w, test_ccr[2], test_timer.ARR)
MDI_PWM_REG_BIND(variable, VARIABLE_CHANNELS)
MDI_STM32_PWM_TIMING_BIND(variable, &test_timer, 170000000U, 2U,                 \
                          VARIABLE_CHANNELS)
MDI_STM32_PWM_LIFECYCLE_FAULT_BIND(
    variable, &test_timer, test_timer.BDTR, 0x8000U,
    test_pwm_fault_latched != 0U, test_pwm_fault_source != 0U,
    test_pwm_fault_latched = 0U)
MDI_FOC_BIND(test_cycle, triggered_current, variable)
#endif
