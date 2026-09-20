/**
 * @file adc_mean.h
 * @brief Compile-time bounded ADC block mean reduction.
 *
 * The DMA owner supplies an interleaved block. This feature reduces one
 * completed block and publishes a stable per-channel snapshot. It does not
 * configure ADC/DMA registers or decide interrupt ownership.
 */
#ifndef MDI_FEATURE_ADC_MEAN_H
#define MDI_FEATURE_ADC_MEAN_H

#include "mdi/feature/adc_dma.h"

#define MDI_ADC_MEAN_JOIN_(A, B) A##B
#define MDI_ADC_MEAN_JOIN(A, B) MDI_ADC_MEAN_JOIN_(A, B)

#define MDI_ADC_MEAN_CHECK(COUNT, NAME, SLOT)                                    \
    _Static_assert((SLOT) >= 0 && (SLOT) < (COUNT),                              \
                   "ADC channel slot out of range");

#define MDI_ADC_MEAN_DECLARE(UNUSED, NAME, SLOT)                                 \
    uint64_t MDI_ADC_MEAN_JOIN(wSum_, NAME) = 0U;

#define MDI_ADC_MEAN_ACCUMULATE(COUNT, NAME, SLOT)                               \
    MDI_ADC_MEAN_JOIN(wSum_, NAME) +=                                            \
        (uint64_t)(pchRaw[(wIndex * (COUNT)) + (SLOT)]);

#define MDI_ADC_MEAN_STORE(COUNT, OUTPUT, NAME, SLOT)                            \
    (OUTPUT)[(SLOT)] = (uint32_t)(                                               \
        MDI_ADC_MEAN_JOIN(wSum_, NAME) / (COUNT));

/**
 * Bind a fixed interleaved 16-bit ADC block to a mean publisher.
 *
 * CHANNELS(X, ...) expands as X(..., channel_name, interleaved_slot). The
 * caller passes SAMPLE_COUNT rows with CHANNEL_COUNT values per row. OUTPUT
 * contains one
 * published mean value per channel. SEQ is made odd while OUTPUT is updated
 * and even after the complete snapshot is visible. Processing is explicit,
 * so it can run in a task or control loop rather than a DMA interrupt.
 */
#define MDI_ADC_MEAN_BIND(NAME, SAMPLE_COUNT, CHANNEL_COUNT, CHANNELS,           \
                          OUTPUT, SEQ)                                           \
    _Static_assert((SAMPLE_COUNT) > 0U, "ADC sample count must be nonzero");     \
    _Static_assert((CHANNEL_COUNT) > 0U, "ADC channel count must be nonzero");   \
    CHANNELS(MDI_ADC_MEAN_CHECK, CHANNEL_COUNT)                                  \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _adc_mean_Process)(                     \
        const volatile uint16_t *pchRaw)                                         \
    {                                                                            \
        uint32_t wIndex;                                                         \
        uint32_t wSequence = (SEQ);                                              \
        CHANNELS(MDI_ADC_MEAN_DECLARE, 0U)                                       \
        if (pchRaw == NULL) { return MDI_INVALID; }                              \
        if ((wSequence & 1U) != 0U) { return MDI_BUSY; }                         \
        (SEQ) = wSequence + 1U;                                                  \
        for (wIndex = 0U; wIndex < (SAMPLE_COUNT); ++wIndex) {                   \
            CHANNELS(MDI_ADC_MEAN_ACCUMULATE, CHANNEL_COUNT)                     \
        }                                                                        \
        CHANNELS(MDI_ADC_MEAN_STORE, SAMPLE_COUNT, OUTPUT)                       \
        (SEQ) = wSequence + 2U;                                                  \
        return MDI_OK;                                                           \
    }

#define MDI_ADC_MeanProcess(R, P) MDI_OP(R, _adc_mean_Process)((P))

/**
 * Bind a mean reducer to a DMA publication resource. The generated Update
 * operation is called by a channel read or a control task, never by the DMA
 * interrupt. VALID is a producer-owned flag for the first completed result.
 */
#define MDI_ADC_MEAN_GROUP_BIND(NAME, DMA, RAW, SAMPLE_COUNT, CHANNEL_COUNT,     \
                                CHANNELS, OUTPUT, SEQ, VALID, RATE_REG,          \
                                CLOCK_HZ)                                        \
    MDI_ADC_MEAN_BIND(NAME, SAMPLE_COUNT, CHANNEL_COUNT, CHANNELS, OUTPUT, SEQ)  \
    MDI_ADC_FREQUENCY_BIND(NAME, RATE_REG, CLOCK_HZ)                             \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _adc_mean_Update)(void)                 \
    {                                                                            \
        uint32_t wIndex;                                                         \
        uint32_t wPublished;                                                     \
        bool bOverrun;                                                           \
        mdi_status_t eStatus;                                                    \
        if (!MDI_ADC_DMA_IsReady(DMA)) {                                         \
            return (VALID) ? MDI_OK : MDI_BUSY;                                  \
        }                                                                        \
        wPublished = MDI_ADC_DMA_Published(DMA);                                 \
        bOverrun = MDI_ADC_DMA_IsOverrun(DMA);                                   \
        wIndex = MDI_ADC_DMA_CompletedIndexAt(DMA, wPublished);                   \
        eStatus = MDI_ADC_MeanProcess(                                           \
            NAME, &(RAW)[wIndex * (SAMPLE_COUNT) * (CHANNEL_COUNT)]);            \
        if (eStatus != MDI_OK) { return eStatus; }                               \
        (VALID) = true;                                                          \
        eStatus = MDI_ADC_DMA_Acknowledge(DMA, wPublished);                      \
        if (eStatus != MDI_OK) { return eStatus; }                               \
        return bOverrun ? MDI_OVERRUN : MDI_OK;                                  \
    }

#define MDI_ADC_MeanUpdate(R) MDI_OP(R, _adc_mean_Update)()

/** A channel view that updates its group's pending block on demand. */
#define MDI_ADC_CHANNEL_MEAN_BIND(NAME, GROUP, VALUE, MASK, SHIFT, SEQ)          \
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
        mdi_status_t eStatus;                                                    \
        uint32_t wSequence;                                                      \
        uint32_t wCode;                                                          \
        if (ptValue == NULL) { return MDI_INVALID; }                             \
        eStatus = MDI_ADC_MeanUpdate(GROUP);                                     \
        if (eStatus != MDI_OK && eStatus != MDI_OVERRUN) {                       \
            return eStatus;                                                       \
        }                                                                        \
        wSequence = (SEQ);                                                       \
        if ((wSequence & 1U) != 0U) { return MDI_BUSY; }                         \
        wCode = (((uint32_t)(VALUE) >> (SHIFT)) & (uint32_t)(MASK));             \
        if ((SEQ) != wSequence) { return MDI_BUSY; }                             \
        ptValue->wCode = wCode;                                                  \
        return eStatus;                                                          \
    }

#endif /* MDI_FEATURE_ADC_MEAN_H */
