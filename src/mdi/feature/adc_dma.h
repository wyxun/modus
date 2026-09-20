/**
 * @file adc_dma.h
 * @brief Optional ADC/DMA acquisition-group feature.
 *
 * This feature exposes compile-time channel views over a published filtered
 * snapshot. It is deliberately outside core because ADC scan scheduling,
 * DMA buffering and block ownership are one particular hardware composition.
 */
#ifndef MDI_FEATURE_ADC_DMA_H
#define MDI_FEATURE_ADC_DMA_H

#include "mdi/core/adc.h"
#define MDI_ADC_IsReady(R) MDI_OP(R, _adc_IsReady)()
#define MDI_ADC_Start(R) MDI_OP(R, _adc_Start)()
#define MDI_ADC_SetSampleFrequency(R, HZ)                                        \
    MDI_OP(R, _adc_SetSampleFrequency)((HZ))

/** Bind the provider's one-shot ADC/DMA trigger operation. */
#define MDI_ADC_TRIGGER_BIND(NAME, START_REG, START_MASK)                        \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _adc_Start)(void)                       \
    {                                                                            \
        (START_REG) |= (START_MASK);                                             \
        return MDI_OK;                                                           \
    }

/** Bind the optional hardware trigger frequency control for one ADC group. */
#define MDI_ADC_FREQUENCY_BIND(NAME, RATE_REG, CLOCK_HZ)                         \
    _Static_assert((CLOCK_HZ) > 0U, "ADC clock must be nonzero");                \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _adc_SetSampleFrequency)(               \
        uint32_t wHz)                                                            \
    {                                                                            \
        if (wHz == 0U || wHz > (CLOCK_HZ)) { return MDI_RANGE; }                 \
        (RATE_REG) = wHz;                                                        \
        return MDI_OK;                                                           \
    }

/**
 * Bind one channel to a stable published value and producer sequence.
 *
 * VALUE is normally one field in a filtered acquisition snapshot. SEQ is a
 * volatile sequence lvalue shared by every channel in one acquisition group.
 * Odd values mean that the producer is updating the snapshot.
 */
#define MDI_ADC_CHANNEL_SEQ_BIND(NAME, VALUE, MASK, SHIFT, SEQ)                  \
    _Static_assert((SHIFT) >= 0 && (SHIFT) < 32,                                 \
                   "ADC shift out of range");                                    \
    _Static_assert((uint32_t)(MASK) <= (UINT32_MAX >> (SHIFT)),                  \
                   "ADC mask out of range");                                     \
    MDI_INLINE bool MDI_OP(NAME, _adc_IsReady)(void)                             \
    {                                                                            \
        return (((SEQ) & 1U) == 0U);                                             \
    }                                                                            \
    MDI_INLINE uint32_t MDI_OP(NAME, _adc_ReadFast)(void)                        \
    {                                                                            \
        return (((uint32_t)(VALUE) >> (SHIFT)) & (uint32_t)(MASK));              \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _adc_Read)(                             \
        mdi_adc_value_t *ptValue)                                                \
    {                                                                            \
        uint32_t wSequence;                                                      \
        uint32_t wCode;                                                          \
        if (ptValue == NULL) { return MDI_INVALID; }                             \
        wSequence = (SEQ);                                                       \
        if ((wSequence & 1U) != 0U) { return MDI_BUSY; }                         \
        wCode = (((uint32_t)(VALUE) >> (SHIFT)) & (uint32_t)(MASK));             \
        if ((SEQ) != wSequence) { return MDI_BUSY; }                             \
        ptValue->wCode = wCode;                                                  \
        return MDI_OK;                                                           \
    }

/** Bind a read-only view over a snapshot published by an ADC feature. */
#define MDI_ADC_CHANNEL_VIEW_BIND(NAME, VALUE, MASK, SHIFT, SEQ, VALID)          \
    _Static_assert((SHIFT) >= 0 && (SHIFT) < 32,                                 \
                   "ADC shift out of range");                                    \
    _Static_assert((uint32_t)(MASK) <= (UINT32_MAX >> (SHIFT)),                  \
                   "ADC mask out of range");                                     \
    MDI_INLINE bool MDI_OP(NAME, _adc_IsReady)(void)                             \
    {                                                                            \
        return (VALID) && (((SEQ) & 1U) == 0U);                                  \
    }                                                                            \
    MDI_INLINE uint32_t MDI_OP(NAME, _adc_ReadFast)(void)                        \
    {                                                                            \
        return (((uint32_t)(VALUE) >> (SHIFT)) & (uint32_t)(MASK));              \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _adc_Read)(                             \
        mdi_adc_value_t *ptValue)                                                \
    {                                                                            \
        uint32_t wSequence;                                                      \
        uint32_t wCode;                                                          \
        if (ptValue == NULL) { return MDI_INVALID; }                             \
        if (!(VALID)) { return MDI_BUSY; }                                       \
        wSequence = (SEQ);                                                       \
        if ((wSequence & 1U) != 0U) { return MDI_BUSY; }                         \
        wCode = (((uint32_t)(VALUE) >> (SHIFT)) & (uint32_t)(MASK));             \
        if ((SEQ) != wSequence) { return MDI_BUSY; }                             \
        ptValue->wCode = wCode;                                                  \
        return MDI_OK;                                                           \
    }

#define MDI_ADC_DMA_Publish(R) MDI_OP(R, _adc_dma_Publish)()
#define MDI_ADC_DMA_IsReady(R) MDI_OP(R, _adc_dma_IsReady)()
#define MDI_ADC_DMA_IsOverrun(R) MDI_OP(R, _adc_dma_IsOverrun)()
#define MDI_ADC_DMA_CompletedIndex(R) MDI_OP(R, _adc_dma_CompletedIndex)()
#define MDI_ADC_DMA_Published(R) MDI_OP(R, _adc_dma_Published)()
#define MDI_ADC_DMA_CompletedIndexAt(R, P) \
    MDI_OP(R, _adc_dma_CompletedIndexAt)((P))
#define MDI_ADC_DMA_Acknowledge(R, P) MDI_OP(R, _adc_dma_Acknowledge)((P))

/**
 * Bind only the DMA publication flag. The DMA ISR calls Publish after the
 * block is complete; filtering and averaging stay in the external consumer.
 */
#define MDI_ADC_DMA_FLAG_BIND(NAME, PUBLISHED, CONSUMED, BUFFER_COUNT)           \
    _Static_assert((BUFFER_COUNT) >= 2U, "ADC DMA needs two buffer slots");      \
    MDI_INLINE void MDI_OP(NAME, _adc_dma_Publish)(void)                         \
    {                                                                            \
        (PUBLISHED) = (uint32_t)((PUBLISHED) + 1U);                              \
    }                                                                            \
    MDI_INLINE bool MDI_OP(NAME, _adc_dma_IsReady)(void)                         \
    {                                                                            \
        return (PUBLISHED) != (CONSUMED);                                        \
    }                                                                            \
    MDI_INLINE bool MDI_OP(NAME, _adc_dma_IsOverrun)(void)                       \
    {                                                                            \
        return (uint32_t)((PUBLISHED) - (CONSUMED)) >                            \
               (uint32_t)(BUFFER_COUNT);                                         \
    }                                                                            \
    MDI_INLINE uint32_t MDI_OP(NAME, _adc_dma_Published)(void)                   \
    {                                                                            \
        return (uint32_t)(PUBLISHED);                                            \
    }                                                                            \
    MDI_INLINE uint32_t MDI_OP(NAME, _adc_dma_CompletedIndexAt)(                  \
        uint32_t wPublished)                                                     \
    {                                                                            \
        return (uint32_t)((wPublished - 1U) % (BUFFER_COUNT));                   \
    }                                                                            \
    MDI_INLINE uint32_t MDI_OP(NAME, _adc_dma_CompletedIndex)(void)              \
    {                                                                            \
        return MDI_OP(NAME, _adc_dma_CompletedIndexAt)((PUBLISHED));             \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _adc_dma_Acknowledge)(                   \
        uint32_t wPublished)                                                     \
    {                                                                            \
        const uint32_t wCurrent = (PUBLISHED);                                  \
        const uint32_t wTargetDelta =                                           \
            (uint32_t)(wPublished - (CONSUMED));                                \
        const uint32_t wCurrentDelta =                                          \
            (uint32_t)(wCurrent - (CONSUMED));                                  \
        if (wTargetDelta == 0U || wTargetDelta > wCurrentDelta) {               \
            return MDI_BUSY;                                                     \
        }                                                                        \
        (CONSUMED) = wPublished;                                                 \
        return MDI_OK;                                                           \
    }

#endif /* MDI_FEATURE_ADC_DMA_H */
