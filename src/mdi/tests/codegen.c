/**
 * @file codegen.c
 * @brief Like-for-like direct and abstracted Cortex-M4 code generation probes.
 * @author Codex
 * @date 2026-09-18
 */
#if defined(MDI_CODEGEN_FIXTURES)
#include "fixtures.h"
/** @brief Direct pin sampling. @return Physical input level. */
uint32_t raw_input(void)
{
    return (test_b_in >> 7U) & 1U;
}

/** @brief Abstract pin sampling. @return Physical input level. */
uint32_t mdi_input(void)
{
    return MDI_IO_Read(sensor_sda);
}

/** @brief Direct split-port bus. @param wValue Data bits. @return None. */
void raw_bus(uint32_t wValue)
{
    uint32_t wA = ((wValue & 1U) << 1U) | ((wValue & 2U) << 6U);
    uint32_t wB = (wValue & 4U) | ((wValue & 8U) << 9U);
    test_a_out = wA | ((0x82U & ~wA) << 16U);
    test_b_out = wB | ((0x1004U & ~wB) << 16U);
}

/** @brief Abstract split-port bus. @param wValue Data bits. @return None. */
void mdi_bus(uint32_t wValue)
{
    MDI_IO_Write(data, wValue);
}

/** @brief Direct masked split-port bus. @param wMask Logical write mask.
 * @param wValue Logical data bits. @return None.
 */
void raw_bus_masked(uint32_t wMask, uint32_t wValue)
{
    uint32_t wMaskA = ((wMask & 1U) << 1U) | ((wMask & 2U) << 6U);
    uint32_t wMaskB = (wMask & 4U) | ((wMask & 8U) << 9U);
    uint32_t wA = ((wValue & wMask & 1U) << 1U) |
                  ((wValue & wMask & 2U) << 6U);
    uint32_t wB = (wValue & wMask & 4U) |
                  ((wValue & wMask & 8U) << 9U);
    test_a_out = wA | ((wMaskA & ~wA) << 16U);
    test_b_out = wB | ((wMaskB & ~wB) << 16U);
}

/** @brief Abstract masked split-port bus. @param wMask Logical write mask.
 * @param wValue Logical data bits. @return None.
 */
void mdi_bus_masked(uint32_t wMask, uint32_t wValue)
{
    MDI_IO_WriteMasked(data, wMask, wValue);
}

/** @brief Direct low-clock/data edge. @param bOne Data bit. @return None. */
void raw_edge(bool bOne)
{
    test_b_out = 1UL << 24U;
    test_b_out = bOne ? (1UL << 7U) : (1UL << 23U);
}

/** @brief Generic software-I2C pin policy. @param bOne Data bit.
 * @return None.
 */
void mdi_edge(bool bOne)
{
    MDI_SoftI2C_PrepareBit(sensor_bus, bOne);
}
#else
#include "mdi/instance.h"

/** @brief Direct GPIO reference. @return None. */
void raw_led(void)
{
    GPIOC->BSRR = 1UL << 22U;
}

/** @brief Logical GPIO probe. @return None. */
void mdi_led(void)
{
    MDI_IO_Write(status_led, 1U);
}

/** @brief Direct completed ADC frame. @param ptFrame Output.
 * @return Completion status.
 */
mdi_status_t raw_sample(MDI_Sample_Frame(phase_current_completed) *ptFrame)
{
    if (ptFrame == NULL) { return MDI_INVALID; }
    if ((ADC1->ISR & ADC_ISR_JEOS) == 0U) { return MDI_BUSY; }
    ptFrame->u = ADC1->JDR1 & 0x0FFFU;
    ptFrame->v = ADC2->JDR2 & 0x0FFFU;
    ptFrame->w = ADC2->JDR1 & 0x0FFFU;
    ADC1->ISR = ADC_ISR_JEOS;
    return MDI_OK;
}

/** @brief Abstract completed ADC frame. @param ptFrame Output.
 * @return Completion status.
 */
mdi_status_t mdi_sample(MDI_Sample_Frame(phase_current_completed) *ptFrame)
{
    return MDI_Sample_ReadCompleted(phase_current_completed, ptFrame);
}

/** @brief Direct prevalidated PWM. @param ptFrame Tick values. @return None. */
void raw_pwm(const MDI_PWM_Frame(bridge) *ptFrame)
{
    TIM1->CCR1 = ptFrame->u;
    TIM1->CCR2 = ptFrame->v;
    TIM1->CCR3 = ptFrame->w;
}

/** @brief Abstract prevalidated PWM. @param ptFrame Tick values.
 * @return None.
 */
void mdi_pwm(const MDI_PWM_Frame(bridge) *ptFrame)
{
    MDI_PWM_StageFast(bridge, ptFrame);
}

/** @brief Check the real ADC trigger has no forwarding call.
 * @return Trigger status.
 */
mdi_status_t mdi_adc_start(void)
{
    return MDI_ADC_Start(adc_mean);
}

/** @brief Check the real bridge stop has no forwarding call.
 * @return Stop status.
 */
mdi_status_t mdi_safe_stop(void)
{
    return MDI_PWM_SafeStop(bridge);
}

/** @brief Check the real bridge fault read has no forwarding call.
 * @return True when a hardware fault is latched or present.
 */
bool mdi_fault_active(void)
{
    return MDI_PWM_FaultActive(bridge);
}

/** @brief Check the real bridge fault acknowledgement has no forwarding call.
 * @return Fault acknowledgement status.
 */
mdi_status_t mdi_fault_clear(void)
{
    return MDI_PWM_ClearFault(bridge);
}
#endif
