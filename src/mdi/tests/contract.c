/**
 * @file contract.c
 * @brief Executable contracts for static MDI capabilities.
 * @author Codex
 * @date 2026-09-18
 */
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "fixtures.h"
#include "mdi/i2c.h"

/* Test-owned fake registers, never used by the hardware binding. */
volatile uint32_t test_a_in;
volatile uint32_t test_a_out;
volatile uint32_t test_b_in;
volatile uint32_t test_b_out;
uint8_t test_eeprom_mem[256];
uint8_t test_eeprom_status;
uint32_t test_eeprom_write_count;
volatile uint32_t test_adc[4];
volatile uint16_t test_adc_dma[24];
volatile uint32_t test_adc_mean[3];
volatile uint32_t test_adc_mean_seq;
volatile uint32_t test_adc_published;
volatile uint32_t test_adc_consumed;
volatile bool test_adc_inject_publish;
volatile uint32_t test_adc_race_mean[3];
volatile uint32_t test_adc_race_mean_seq;
volatile bool test_adc_race_mean_valid;
volatile uint32_t test_adc_race_rate;
volatile uint32_t test_sample_seq;
volatile uint32_t test_sample_ready;
volatile uint32_t test_pwm_fault_latched;
volatile uint32_t test_pwm_fault_source;
volatile uint32_t test_ccr[4];
volatile mdi_tick_t test_raw_tick_counter;
test_timer_t test_timer;
volatile uint32_t test_timer_frequency;
volatile bool test_timer_running;
volatile uint32_t test_timer_start_count;
volatile uint32_t test_timer_stop_count;

static bool test_RawTickElapsed(mdi_tick_t wLimit)
{
    return MDI_TICK_Elapsed(test_raw_tick, wLimit);
}

static void test_TimerAndRawTick(void)
{
    test_timer_frequency = 0U;
    test_timer_running = false;
    test_timer_start_count = 0U;
    test_timer_stop_count = 0U;
    assert(MDI_TIMER_SetFrequency(test_timer_resource, 0U) == MDI_RANGE);
    assert(MDI_TIMER_SetFrequency(test_timer_resource, 1000U) == MDI_OK);
    assert(test_timer_frequency == 1000U);
    assert(MDI_TIMER_Start(test_timer_resource) == MDI_OK);
    assert(MDI_TIMER_IsRunning(test_timer_resource));
    assert(MDI_TIMER_SetFrequency(test_timer_resource, 2000U) == MDI_BUSY);
    assert(MDI_TIMER_Stop(test_timer_resource) == MDI_OK);
    assert(!MDI_TIMER_IsRunning(test_timer_resource));
    assert(test_timer_start_count == 1U && test_timer_stop_count == 1U);

    test_raw_tick_counter = UINT64_MAX - 2U;
    assert(!test_RawTickElapsed(3U));
    test_raw_tick_counter = 1U;
    assert(test_RawTickElapsed(3U));
}

volatile uint16_t *test_adc_race_raw(void)
{
    if (test_adc_inject_publish) {
        test_adc_inject_publish = false;
        MDI_ADC_DMA_Publish(test_adc_dma);
    }
    return test_adc_dma;
}

/** @brief Check digital mappings and argument evaluation.
 * @return None.
 */
static void test_Io(void)
{
    uint32_t wValue = 5U;
    MDI_IO_Write(led, 1U);
    assert(test_a_out == (1UL << 22U));
    MDI_IO_Write(led, 0U);
    assert(test_a_out == (1UL << 6U));
    test_a_in = 0U;
    assert(MDI_IO_Read(led) == 1U);
    test_a_in = 1UL << 6U;
    assert(MDI_IO_Read(led) == 0U);
    MDI_IO_Write(data, wValue++);
    assert(wValue == 6U);
    assert(test_a_out == ((1UL << 1U) | (1UL << 23U)));
    assert(test_b_out == ((1UL << 2U) | (1UL << 28U)));
    test_a_in = (1UL << 1U) | (1UL << 7U);
    test_b_in = 1UL << 12U;
    assert(MDI_IO_Read(data) == 11U);
    MDI_IO_Write(data, UINT32_MAX);
    assert(test_a_out == ((1UL << 1U) | (1UL << 7U)));
    assert(test_b_out == ((1UL << 2U) | (1UL << 12U)));
    MDI_IO_Write(data, 0U);
    MDI_IO_WriteMasked(data, 0x5U, 0x5U);
    assert(test_a_out == (1UL << 1U));
    assert(test_b_out == (1UL << 2U));
    MDI_IO_WriteMasked(data, 0x5U, 0x0U);
    assert(test_a_out == (1UL << 17U));
    assert(test_b_out == (1UL << 18U));
    MDI_IO_WriteMasked(data, 0x5U, 0xAU);
    assert(test_a_out == (1UL << 17U));
    assert(test_b_out == (1UL << 18U));
}

/** @brief Check compile-time IO mode capabilities exposed by a binding.
 * @return None.
 */
static void test_IoCapabilities(void)
{
    assert((MDI_IO_Capabilities(led) & MDI_IO_CAP_OUTPUT) != 0U);
    assert((MDI_IO_Capabilities(sensor_sda) & MDI_IO_CAP_OPEN_DRAIN) != 0U);
    assert((MDI_IO_Capabilities(sensor_scl) & MDI_IO_CAP_INPUT) != 0U);
}

/** @brief Check scalar and heterogeneous sample frames.
 * @return None.
 */
static void test_Sample(void)
{
    MDI_Sample_Frame(current) tFrame = {0};
    MDI_Sample_Frame(sensor) tSensor = {0};
    test_adc[0] = 0xFFFF1234U;
    test_adc[1] = 0x9876U;
    test_adc[2] = 0xFEDCU;
    test_adc[3] = 0xAB00U;
    MDI_Sample_Read(current, &tFrame);
    assert(tFrame.u == 0x1234U);
    assert(tFrame.v == 0xFEDCU);
    assert(tFrame.w == 0x9876U);
    MDI_Sample_Read(sensor, &tSensor);
    assert(tSensor.value == 0xABU);
    {
        mdi_adc_value_t tValue = {0};
        assert(MDI_ADC_Read(raw_adc, &tValue) == MDI_OK);
        assert(tValue.wCode == 0xABU);
    }
}

/** @brief Reject an in-progress or changed producer frame.
 * @return None.
 */
static void test_StableSample(void)
{
    MDI_Sample_Frame(stable_current) tFrame = {0};
    test_sample_seq = 2U;
    test_adc[0] = 11U;
    test_adc[2] = 22U;
    test_adc[1] = 33U;
    assert(MDI_Sample_ReadStable(stable_current, &tFrame) == MDI_OK);
    assert(tFrame.u == 11U && tFrame.v == 22U && tFrame.w == 33U);
    test_sample_seq = 1U;
    assert(MDI_Sample_ReadStable(stable_current, &tFrame) == MDI_BUSY);
    assert(MDI_Sample_ReadStable(stable_current, NULL) == MDI_INVALID);
}

/** @brief Check DMA publication, on-demand block reduction and channel views.
 * @return None.
 */
static void test_AdcDmaMean(void)
{
    mdi_adc_value_t tValue = {0};
    uint32_t wIndex;
    test_adc_published = 0U;
    test_adc_consumed = 0U;
    test_adc_mean_seq = 0U;
    for (wIndex = 0U; wIndex < 12U; ++wIndex) {
        test_adc_dma[wIndex] = (uint16_t)(wIndex + 1U);
    }
    assert(!MDI_ADC_DMA_IsReady(test_adc_dma));
    MDI_ADC_DMA_Publish(test_adc_dma);
    assert(MDI_ADC_DMA_IsReady(test_adc_dma));
    assert(!MDI_ADC_DMA_IsOverrun(test_adc_dma));
    assert(MDI_ADC_DMA_CompletedIndex(test_adc_dma) == 0U);
    assert(MDI_ADC_MeanProcess(test_adc_mean_filter, test_adc_dma) == MDI_OK);
    assert(MDI_ADC_Read(mean_u, &tValue) == MDI_OK);
    assert(tValue.wCode == 5U);
    assert(MDI_ADC_Read(mean_v, &tValue) == MDI_OK);
    assert(tValue.wCode == 6U);
    assert(MDI_ADC_Read(mean_w, &tValue) == MDI_OK);
    assert(tValue.wCode == 7U);
    assert(MDI_ADC_DMA_Acknowledge(test_adc_dma, 1U) == MDI_OK);
    assert(!MDI_ADC_DMA_IsReady(test_adc_dma));

    /* A publication during reduction remains pending for the next pass. */
    test_adc_published = 1U;
    test_adc_consumed = 0U;
    MDI_ADC_DMA_Publish(test_adc_dma);
    assert(MDI_ADC_DMA_Acknowledge(test_adc_dma, 1U) == MDI_OK);
    assert(test_adc_consumed == 1U);
    assert(MDI_ADC_DMA_IsReady(test_adc_dma));
    assert(MDI_ADC_DMA_CompletedIndexAt(test_adc_dma, 2U) == 1U);

    test_adc_published = 4U;
    test_adc_consumed = 1U;
    assert(MDI_ADC_DMA_IsOverrun(test_adc_dma));
    assert(MDI_ADC_Read(mean_u, &tValue) == MDI_OK);
    test_adc_mean_seq = 3U;
    assert(MDI_ADC_Read(mean_u, &tValue) == MDI_BUSY);
}

/** @brief Check DMA race retention, overflow recovery and sequence wrap.
 * @return None.
 */
static void test_AdcDmaRecovery(void)
{
    uint32_t wIndex;

    for (wIndex = 0U; wIndex < 24U; ++wIndex) {
        test_adc_dma[wIndex] = (uint16_t)(100U + wIndex);
    }
    test_adc_mean_seq = 0U;
    test_adc_race_mean_seq = 0U;
    test_adc_race_mean_valid = false;
    test_adc_published = 1U;
    test_adc_consumed = 0U;
    test_adc_inject_publish = true;
    assert(MDI_ADC_MeanUpdate(test_adc_race_mean) == MDI_OK);
    assert(test_adc_published == 2U && test_adc_consumed == 1U);
    assert(MDI_ADC_DMA_IsReady(test_adc_dma));

    test_adc_published = 3U;
    test_adc_consumed = 0U;
    test_adc_inject_publish = false;
    assert(MDI_ADC_MeanUpdate(test_adc_race_mean) == MDI_OVERRUN);
    assert(test_adc_consumed == 3U);
    assert(!MDI_ADC_DMA_IsReady(test_adc_dma));
    assert(test_adc_race_mean_valid);

    test_adc_published = 0U;
    test_adc_consumed = 0U;
    assert(MDI_ADC_DMA_Acknowledge(test_adc_dma, 1U) == MDI_BUSY);
    assert(test_adc_consumed == 0U);

    test_adc_published = UINT32_MAX;
    test_adc_consumed = UINT32_MAX - 1U;
    assert(MDI_ADC_DMA_Acknowledge(test_adc_dma, UINT32_MAX) == MDI_OK);
    test_adc_published = 0U;
    assert(MDI_ADC_DMA_Acknowledge(test_adc_dma, 0U) == MDI_OK);
    assert(test_adc_consumed == 0U);
}

/** @brief Check hardware-I2C timeout conversion and error precedence.
 * @return None.
 */
static void test_Stm32I2cContract(void)
{
    I2C_TypeDef tI2c = {0};

    assert(MDI_STM32_I2C_CLOCK_HZ == 170000000U);
    assert(MDI_STM32_I2C_POLL_CYCLES == 32U);
    assert(MDI_STM32_I2C_POLLS_PER_US == 5U);
    assert(mdi_stm32_i2c_PollBudget(10U, 3U, 100U) == 30U);
    assert(mdi_stm32_i2c_PollBudget(100U, 3U, 100U) == 100U);
    assert(mdi_stm32_i2c_PollBudget(0U, 3U, 100U) == 0U);
    tI2c.ISR = I2C_ISR_STOPF | I2C_ISR_NACKF;
    assert(mdi_stm32_i2c_wait(&tI2c, I2C_ISR_STOPF, 10U) ==
           MDI_IO_ERROR);
}

/** @brief Check completion-flag ownership around a three-phase frame.
 * @return None.
 */
static void test_CompletedSample(void)
{
    MDI_Sample_Frame(triggered_current) tFrame = {0};
    test_sample_ready = 0U;
    assert(!MDI_Sample_IsReady(triggered_current));
    assert(MDI_Sample_ReadCompleted(triggered_current, &tFrame) ==
           MDI_BUSY);
    test_sample_ready = 1U;
    test_adc[0] = 101U;
    test_adc[2] = 202U;
    test_adc[1] = 303U;
    assert(MDI_Sample_ReadCompleted(triggered_current, &tFrame) ==
           MDI_OK);
    assert(tFrame.u == 101U && tFrame.v == 202U && tFrame.w == 303U);
    assert(test_sample_ready == 0U);
    assert(MDI_Sample_ReadCompleted(triggered_current, NULL) ==
           MDI_INVALID);
}

/** @brief Check one complete FOC sample-to-duty cycle transaction.
 * @return None.
 */
static void test_FocCycle(void)
{
    MDI_Sample_Frame(triggered_current) tSample = {0};
    const MDI_PWM_DutyFrame(variable) tDuty = {
        .u = 16384U, .v = 32768U, .w = 49152U
    };
    test_timer.ARR = 4250U;
    test_timer.EGR = 0U;
    test_sample_ready = 0U;
    assert(MDI_FOC_RunCycle(test_cycle, &tSample, &tDuty) == MDI_BUSY);
    assert(test_timer.EGR == 0U);
    test_sample_ready = 1U;
    test_adc[0] = 1U;
    test_adc[2] = 2U;
    test_adc[1] = 3U;
    assert(MDI_FOC_RunCycle(test_cycle, &tSample, &tDuty) == MDI_OK);
    assert(tSample.u == 1U && tSample.v == 2U && tSample.w == 3U);
    assert(test_ccr[0] == 1063U && test_ccr[1] == 2125U &&
           test_ccr[2] == 3188U);
    assert(test_timer.EGR == 1U);

    {
        const MDI_PWM_Frame(variable) tTicks = {.u = 17U, .v = 29U,
                                                .w = 41U};
        test_sample_ready = 1U;
        test_timer.EGR = 0U;
        assert(MDI_FOC_RunCycleFast(test_cycle, &tSample, &tTicks) ==
               MDI_OK);
        assert(test_ccr[0] == 17U && test_ccr[1] == 29U &&
               test_ccr[2] == 41U);
        assert(test_timer.EGR == 1U);
    }
}

/** @brief Reject the entire PWM frame before any register is written.
 * @return None.
 */
static void test_Pwm(void)
{
    MDI_PWM_Frame(bridge) tDuty = {.u = 10U, .v = 20U, .w = 30U};
    MDI_PWM_Frame(buzzer) tSingle = {.value = 77U};
    assert(MDI_PWM_Stage(bridge, &tDuty) == MDI_OK);
    assert(test_ccr[0] == 10U && test_ccr[1] == 20U);
    assert(test_ccr[2] == 30U);
    tDuty.w = 4251U;
    tDuty.u = 99U;
    assert(MDI_PWM_Stage(bridge, &tDuty) == MDI_RANGE);
    assert(test_ccr[0] == 10U && test_ccr[2] == 30U);
    assert(MDI_PWM_Stage(bridge, NULL) == MDI_INVALID);
    tDuty.w = 4250U;
    tDuty.v = 0U;
    MDI_PWM_StageFast(bridge, &tDuty);
    assert(test_ccr[0] == 99U && test_ccr[1] == 0U);
    assert(test_ccr[2] == 4250U);
    assert(MDI_PWM_Stage(buzzer, &tSingle) == MDI_OK);
    assert(test_ccr[3] == 77U);
}

/** @brief Frequency changes update duty scaling and reject running changes.
 * @return None.
 */
static void test_Frequency(void)
{
    MDI_PWM_DutyFrame(variable) tDuty = {
        .u = 32768U, .v = 65536U, .w = 0U
    };
    test_timer.CR1 = 0x20U;
    assert(MDI_PWM_SetFrequency(variable, 20000U) == MDI_OK);
    assert(test_timer.ARR == 4250U && test_timer.PSC == 0U);
    assert(test_ccr[0] == 0U && test_ccr[1] == 0U);
    assert(test_timer.EGR == 1U);
    assert(MDI_PWM_SetDuty(variable, &tDuty) == MDI_OK);
    assert(test_ccr[0] == 2125U && test_ccr[1] == 4250U);
    assert(MDI_PWM_SetFrequency(variable, 40000U) == MDI_OK);
    assert(MDI_PWM_SetDuty(variable, &tDuty) == MDI_OK);
    assert(test_ccr[0] == 1063U && test_ccr[1] == 2125U);
    test_timer.CR1 |= 1U;
    assert(MDI_PWM_SetFrequency(variable, 10000U) == MDI_BUSY);
    assert(test_timer.ARR == 2125U);
    test_timer.CR1 = 0x20U;
    assert(MDI_PWM_SetFrequency(variable, 0U) == MDI_RANGE);
    assert(MDI_PWM_SetFrequency(variable, UINT32_MAX) == MDI_RANGE);
    assert(test_timer.ARR == 2125U);
    tDuty.v = 65537U;
    assert(MDI_PWM_SetDuty(variable, &tDuty) == MDI_RANGE);
    assert(test_ccr[1] == 2125U);
    assert(MDI_PWM_SetDuty(variable, NULL) == MDI_INVALID);
    {
        const MDI_PWM_Cycle(variable) tCycle = {
            .wPeriodTicks = 1000U,
            .tCompare = {.u = 100U, .v = 500U, .w = 1000U}
        };
        test_timer.CR1 = 0xA1U;
        test_timer.EGR = 0U;
        assert(MDI_PWM_StageCycle(variable, &tCycle) == MDI_OK);
        assert(test_timer.ARR == 1000U && test_ccr[1] == 500U);
        assert(test_timer.EGR == 0U);
        assert(MDI_PWM_Commit(variable) == MDI_OK);
        assert(test_timer.EGR == 1U);
    }
}

/** @brief Check gated PWM enable order and idempotent safe stop.
 * @return None.
 */
static void test_PwmLifecycle(void)
{
    test_timer.CR1 = 0U;
    test_timer.BDTR = 0U;
    test_pwm_fault_latched = 0U;
    test_pwm_fault_source = 0U;
    assert(!MDI_PWM_IsEnabled(variable));
    test_pwm_fault_latched = 1U;
    assert(MDI_PWM_FaultActive(variable));
    assert(MDI_PWM_Enable(variable, true) == MDI_BUSY);
    assert(!MDI_PWM_IsEnabled(variable));
    test_pwm_fault_source = 1U;
    assert(MDI_PWM_ClearFault(variable) == MDI_BUSY);
    assert(MDI_PWM_FaultActive(variable));
    test_pwm_fault_source = 0U;
    assert(MDI_PWM_ClearFault(variable) == MDI_OK);
    assert(!MDI_PWM_FaultActive(variable));
    assert(MDI_PWM_Enable(variable, true) == MDI_OK);
    assert((test_timer.CR1 & 1U) != 0U);
    assert((test_timer.BDTR & 0x8000U) != 0U);
    assert(MDI_PWM_IsEnabled(variable));
    assert(MDI_PWM_SafeStop(variable) == MDI_OK);
    assert((test_timer.CR1 & 1U) == 0U);
    assert((test_timer.BDTR & 0x8000U) == 0U);
    assert(!MDI_PWM_IsEnabled(variable));
}

/** @brief Check independent open-drain line operations for software I2C.
 * @return None.
 */
static void test_SoftI2cEdges(void)
{
    MDI_SoftI2C_SdaLow(sensor_bus);
    assert(test_b_out == (1UL << 23U));
    MDI_SoftI2C_SdaRelease(sensor_bus);
    assert(test_b_out == (1UL << 7U));
    MDI_SoftI2C_SclLow(sensor_bus);
    assert(test_b_out == (1UL << 24U));
    MDI_SoftI2C_SclRelease(sensor_bus);
    assert(test_b_out == (1UL << 8U));
    test_b_in = 1UL << 8U;
    assert(MDI_SoftI2C_ReadScl(sensor_bus));
    test_b_in = 0U;
    assert(!MDI_SoftI2C_ReadSda(sensor_bus));
}

/** @brief Check a bounded, statically bound software-I2C write transaction.
 * @return None.
 */
static void test_SoftI2cTransfer(void)
{
    const uint8_t chTx[] = {0x10U, 0x5AU};
    const mdi_i2c_transfer_t tWrite = {
        .pchTx = chTx,
        .wTxLength = sizeof(chTx),
        .pchRx = NULL,
        .wRxLength = 0U,
        .wTimeoutUs = 100U,
        .hwAddress7 = 0x36U
    };
    test_b_in = 1UL << 8U;
    assert(MDI_I2C_Transfer(sensor_i2c, &tWrite) == MDI_OK);
    assert(MDI_I2C_Transfer(sensor_i2c, NULL) == MDI_INVALID);
    {
        const mdi_i2c_transfer_t tBad = {
            .pchTx = chTx,
            .wTxLength = sizeof(chTx),
            .wTimeoutUs = 100U,
            .hwAddress7 = 0x80U
        };
        assert(MDI_I2C_Transfer(sensor_i2c, &tBad) == MDI_RANGE);
    }
    test_b_in = 0U;
    assert(MDI_I2C_Transfer(sensor_i2c, &tWrite) == MDI_TIMEOUT);
}

/** @brief Check the old register-command IIC use case on the new transfer API.
 * @return None.
 */
static void test_I2cRegisterDevice(void)
{
    uint8_t chValue = 0xFFU;
    test_b_in = 1UL << 8U;
    assert(MDI_I2C_Reg8_WriteByte(sensor_reg, 0x0FU, 0xA5U) == MDI_OK);
    assert(MDI_I2C_Reg8_Read(sensor_reg, 0x0FU, &chValue, 1U) == MDI_OK);
    assert(chValue == 0U);
}

/** @brief Check page splitting and full-duplex SPI EEPROM transactions.
 * @return None.
 */
static void test_SpiEeprom(void)
{
    uint8_t chWrite[40];
    uint8_t chRead[40] = {0};
    uint32_t wIndex;
    memset(test_eeprom_mem, 0xFF, sizeof(test_eeprom_mem));
    test_eeprom_status = 0U;
    test_eeprom_write_count = 0U;
    for (wIndex = 0U; wIndex < sizeof(chWrite); ++wIndex) {
        chWrite[wIndex] = (uint8_t)(wIndex + 1U);
    }
    assert(MDI_SPI_EEPROM_Write(eeprom, 0x003EU, chWrite,
                                sizeof(chWrite)) == MDI_OK);
    assert(test_eeprom_write_count == 4U);
    assert(MDI_SPI_EEPROM_Read(eeprom, 0x003EU, chRead,
                               sizeof(chRead)) == MDI_OK);
    assert(memcmp(chWrite, chRead, sizeof(chWrite)) == 0);
    assert(MDI_SPI_EEPROM_WaitReady(eeprom, 2U) == MDI_OK);
    assert(MDI_SPI_EEPROM_Read(eeprom, 220U, chRead, 40U) == MDI_RANGE);
    assert(MDI_SPI_EEPROM_Write(eeprom, 0U, NULL, 1U) == MDI_INVALID);
}

/** @brief Run contracts without initializing or accessing a real MCU.
 * @return Zero on success.
 */
int main(void)
{
    test_TimerAndRawTick();
    test_Io();
    test_IoCapabilities();
    test_Sample();
    test_StableSample();
    test_AdcDmaMean();
    test_AdcDmaRecovery();
    test_Stm32I2cContract();
    test_CompletedSample();
    test_FocCycle();
    test_Pwm();
    test_Frequency();
    test_PwmLifecycle();
    test_SoftI2cEdges();
    test_SoftI2cTransfer();
    test_I2cRegisterDevice();
    test_SpiEeprom();
    return 0;
}
