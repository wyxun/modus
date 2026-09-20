/**
 * @file bind.h
 * @brief Reusable providers for side-effect-free sample registers and PWM.
 * @author Codex
 * @date 2026-09-18
 * @note Lists expand to typed functions, not descriptor arrays. Registers
 * must be native-width volatile lvalues. Read-to-clear/FIFO registers need
 * their own provider. No initialization or ownership is implied.
 */
#ifndef MDI_CORE_BIND_H
#define MDI_CORE_BIND_H
#include "mdi/core/contract.h"

#define MDI_CORE_SAMPLE_FIELD(N, R, M, S) uint32_t N;
#define MDI_CORE_SAMPLE_CHECK(N, R, M, S)                                        \
    _Static_assert((S) >= 0 && (S) < 32, "sample shift out of range");           \
    _Static_assert((uint32_t)(M) <= (UINT32_MAX >> (S)),                         \
                   "sample mask out of range");
#define MDI_CORE_SAMPLE_READ(N, R, M, S)                                         \
    ptFrame->N = ((uint32_t)(R) >> (S)) & (uint32_t)(M);

/** @brief Bind named fields to already-completed sample registers.
 * @param NAME Resource token.
 * @param CHANNELS X-list: field, register, output mask, right shift.
 * @return Generates NAME_sample_frame_t and NAME_sample_Read().
 * @note Read requires nonnull writable output and stable source ownership.
 */
#define MDI_SAMPLE_REG_BIND(NAME, CHANNELS)                                      \
    CHANNELS(MDI_CORE_SAMPLE_CHECK)                                              \
    typedef struct {                                                             \
        CHANNELS(MDI_CORE_SAMPLE_FIELD)                                          \
    } MDI_Sample_Frame(NAME);                                                    \
    MDI_INLINE void MDI_OP(NAME, _sample_Read)(                                  \
        MDI_Sample_Frame(NAME) *ptFrame)                                         \
    {                                                                            \
        CHANNELS(MDI_CORE_SAMPLE_READ)                                           \
    }

/** @brief Bind a frame with an even/unchanged producer sequence marker.
 * @param NAME Resource token.
 * @param CHANNELS X-list: field, register, output mask, right shift.
 * @param SEQ Volatile producer sequence expression; odd means write in progress.
 * @return Generates ordinary and stable frame reads.
 * @note The producer must publish an even sequence after the complete frame is
 * written. This checks ownership consistency without a runtime frame object.
 */
#define MDI_SAMPLE_SEQ_BIND(NAME, CHANNELS, SEQ)                                 \
    CHANNELS(MDI_CORE_SAMPLE_CHECK)                                              \
    typedef struct {                                                             \
        CHANNELS(MDI_CORE_SAMPLE_FIELD)                                          \
    } MDI_Sample_Frame(NAME);                                                    \
    MDI_INLINE void MDI_OP(NAME, _sample_Read)(                                  \
        MDI_Sample_Frame(NAME) *ptFrame)                                         \
    {                                                                            \
        CHANNELS(MDI_CORE_SAMPLE_READ)                                           \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _sample_ReadStable)(                    \
        MDI_Sample_Frame(NAME) *ptFrame)                                         \
    {                                                                            \
        uint32_t wSequence;                                                      \
        if (ptFrame == NULL) { return MDI_INVALID; }                             \
        wSequence = (SEQ);                                                       \
        if ((wSequence & 1U) != 0U) { return MDI_BUSY; }                         \
        CHANNELS(MDI_CORE_SAMPLE_READ)                                           \
        if ((SEQ) != wSequence) { return MDI_BUSY; }                             \
        return MDI_OK;                                                           \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _sample_ReadCompleted)(                 \
        MDI_Sample_Frame(NAME) *ptFrame)                                         \
    {                                                                            \
        return MDI_OP(NAME, _sample_ReadStable)(ptFrame);                        \
    }

/** @brief Bind a frame to a hardware/DMA completion flag.
 * @param NAME Resource token.
 * @param CHANNELS X-list: field, register, output mask, right shift.
 * @param READY Volatile completion expression, nonzero when the frame is ready.
 * @param CLEAR Completion acknowledgement expression executed after the read.
 * @return Generates readiness and completed-frame operations.
 * @note This never waits or starts conversion. The owner decides when it is
 * safe to acknowledge the completion source.
 */
#define MDI_SAMPLE_READY_BIND(NAME, CHANNELS, READY, CLEAR)                      \
    CHANNELS(MDI_CORE_SAMPLE_CHECK)                                              \
    typedef struct {                                                             \
        CHANNELS(MDI_CORE_SAMPLE_FIELD)                                          \
    } MDI_Sample_Frame(NAME);                                                    \
    MDI_INLINE bool MDI_OP(NAME, _sample_IsReady)(void)                          \
    {                                                                            \
        return (READY) != 0U;                                                    \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _sample_ReadCompleted)(                 \
        MDI_Sample_Frame(NAME) *ptFrame)                                         \
    {                                                                            \
        if (ptFrame == NULL) { return MDI_INVALID; }                             \
        if ((READY) == 0U) { return MDI_BUSY; }                                  \
        CHANNELS(MDI_CORE_SAMPLE_READ)                                           \
        (CLEAR);                                                                 \
        return MDI_OK;                                                           \
    }

#define MDI_CORE_PWM_FIELD(N, R, MAX) uint32_t N;
#define MDI_CORE_PWM_INVALID(N, R, MAX) || (ptFrame->N > (uint32_t)(MAX))
#define MDI_CORE_PWM_WRITE(N, R, MAX) (R) = ptFrame->N;

/** @brief Bind compare fields without assuming a timer topology.
 * @param NAME Resource token.
 * @param CHANNELS X-list: field, compare register, inclusive tick maximum.
 * @return Generates a frame and checked/fast stage operations.
 * @note MAX may be a live period expression when frequency is mutable.
 * Caller serializes reconfiguration with staging. Fast requires valid data.
 * Preload and update timing must be established before either operation.
 */
#define MDI_PWM_REG_BIND(NAME, CHANNELS)                                         \
    typedef struct {                                                             \
        CHANNELS(MDI_CORE_PWM_FIELD)                                             \
    } MDI_PWM_Frame(NAME);                                                       \
    MDI_INLINE void MDI_OP(NAME, _pwm_StageFast)(                                \
        const MDI_PWM_Frame(NAME) *ptFrame)                                      \
    {                                                                            \
        CHANNELS(MDI_CORE_PWM_WRITE)                                             \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _pwm_Stage)(                            \
        const MDI_PWM_Frame(NAME) *ptFrame)                                      \
    {                                                                            \
        if (ptFrame == NULL) { return MDI_INVALID; }                             \
        if (false CHANNELS(MDI_CORE_PWM_INVALID)) { return MDI_RANGE; }          \
        MDI_PWM_StageFast(NAME, ptFrame);                                        \
        return MDI_OK;                                                           \
    }
#endif
