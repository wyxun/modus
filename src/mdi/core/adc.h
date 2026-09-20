/**
 * @file adc.h
 * @brief Core ADC value contract.
 *
 * This header only describes reading one already-bound ADC value. Acquisition
 * groups, DMA ownership and filtering are optional feature-layer policies.
 */
#ifndef MDI_CORE_ADC_H
#define MDI_CORE_ADC_H

#include "mdi/core/contract.h"

typedef struct {
    uint32_t wCode;
} mdi_adc_value_t;

#define MDI_ADC_Read(R, P) MDI_OP(R, _adc_Read)((P))
#define MDI_ADC_ReadFast(R) MDI_OP(R, _adc_ReadFast)()

/** Bind one stable ADC register or provider value to the core read contract. */
#define MDI_ADC_CHANNEL_BIND(NAME, VALUE, MASK, SHIFT)                           \
    _Static_assert((SHIFT) >= 0 && (SHIFT) < 32,                                 \
                   "ADC shift out of range");                                    \
    _Static_assert((uint32_t)(MASK) <= (UINT32_MAX >> (SHIFT)),                  \
                   "ADC mask out of range");                                     \
    MDI_INLINE uint32_t MDI_OP(NAME, _adc_ReadFast)(void)                        \
    {                                                                            \
        return (((uint32_t)(VALUE) >> (SHIFT)) & (uint32_t)(MASK));              \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _adc_Read)(                             \
        mdi_adc_value_t *ptValue)                                                \
    {                                                                            \
        if (ptValue == NULL) { return MDI_INVALID; }                             \
        ptValue->wCode = MDI_OP(NAME, _adc_ReadFast)();                          \
        return MDI_OK;                                                           \
    }

#endif /* MDI_CORE_ADC_H */
