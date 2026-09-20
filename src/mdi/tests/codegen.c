/**
 * @file codegen.c
 * @brief Like-for-like direct and abstracted Cortex-M4 code generation probes.
 * @author Codex
 * @date 2026-09-18
 */
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

/** @brief Direct pin sampling. @return Physical input level. */
uint32_t raw_input(void)
{
    return (GPIOB->IDR >> 7U) & 1U;
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
    GPIOA->BSRR = wA | ((0x82U & ~wA) << 16U);
    GPIOB->BSRR = wB | ((0x1004U & ~wB) << 16U);
}

/** @brief Abstract split-port bus. @param wValue Data bits. @return None. */
void mdi_bus(uint32_t wValue)
{
    MDI_IO_Write(dac_data, wValue);
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
    GPIOA->BSRR = wA | ((wMaskA & ~wA) << 16U);
    GPIOB->BSRR = wB | ((wMaskB & ~wB) << 16U);
}

/** @brief Abstract masked split-port bus. @param wMask Logical write mask.
 * @param wValue Logical data bits. @return None.
 */
void mdi_bus_masked(uint32_t wMask, uint32_t wValue)
{
    MDI_IO_WriteMasked(dac_data, wMask, wValue);
}

/** @brief Direct ADC frame. @param ptFrame Output. @return None. */
void raw_sample(MDI_Sample_Frame(phase_current) *ptFrame)
{
    ptFrame->u = (uint16_t)ADC1->JDR1;
    ptFrame->v = (uint16_t)ADC2->JDR2;
    ptFrame->w = (uint16_t)ADC2->JDR1;
}

/** @brief Abstract ADC frame. @param ptFrame Output. @return None. */
void mdi_sample(MDI_Sample_Frame(phase_current) *ptFrame)
{
    MDI_Sample_Read(phase_current, ptFrame);
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

/** @brief Direct low-clock/data edge. @param bOne Data bit. @return None. */
void raw_edge(bool bOne)
{
    GPIOB->BSRR = 1UL << 24U;
    GPIOB->BSRR = bOne ? (1UL << 7U) : (1UL << 23U);
}

/** @brief Generic software-I2C pin policy. @param bOne Data bit.
 * @return None.
 */
void mdi_edge(bool bOne)
{
    MDI_SoftI2C_PrepareBit(sensor_bus, bOne);
}
