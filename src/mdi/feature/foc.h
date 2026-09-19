/**
 * @file foc.h
 * @brief Optional static composition of one FOC sample/update cycle.
 * @author Codex
 * @date 2026-09-18
 * @note This is a domain composition helper, not a motor-control algorithm.
 * It consumes a completed sample frame, stages a normalized or prevalidated
 * tick frame and requests the PWM update event without a runtime object.
 */
#ifndef MDI_FEATURE_FOC_H
#define MDI_FEATURE_FOC_H
#include "mdi/core/contract.h"

#define MDI_FOC_RunCycle(R, S, D) MDI_OP(R, _foc_RunCycle)((S), (D))
#define MDI_FOC_RunCycleFast(R, S, D) MDI_OP(R, _foc_RunCycleFast)((S), (D))

/** @brief Bind a completed ADC frame to a PWM duty/commit group.
 * @param NAME FOC resource token.
 * @param SAMPLE Completed sample resource token.
 * @param PWM PWM resource token with SetDuty and Commit capabilities.
 * @return Generates one bounded sample-to-update operation.
 * @note A busy/incomplete sample leaves the PWM untouched. The PWM provider
 * remains responsible for preload configuration and safety gating.
 * The generated Fast operation accepts a prevalidated timer-tick frame and
 * skips duty conversion and range checks for a bounded real-time path.
 */
#define MDI_FOC_BIND(NAME, SAMPLE, PWM)                                          \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _foc_RunCycle)(                         \
        MDI_Sample_Frame(SAMPLE) *ptSample,                                      \
        const MDI_PWM_DutyFrame(PWM) *ptDuty)                                    \
    {                                                                            \
        mdi_status_t eStatus;                                                    \
        eStatus = MDI_Sample_ReadCompleted(SAMPLE, ptSample);                    \
        if (eStatus != MDI_OK) { return eStatus; }                               \
        eStatus = MDI_PWM_SetDuty(PWM, ptDuty);                                  \
        if (eStatus != MDI_OK) { return eStatus; }                               \
        return MDI_PWM_Commit(PWM);                                              \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _foc_RunCycleFast)(                     \
        MDI_Sample_Frame(SAMPLE) *ptSample,                                      \
        const MDI_PWM_Frame(PWM) *ptCompare)                                     \
    {                                                                            \
        mdi_status_t eStatus;                                                    \
        eStatus = MDI_Sample_ReadCompleted(SAMPLE, ptSample);                    \
        if (eStatus != MDI_OK) { return eStatus; }                               \
        MDI_PWM_StageFast(PWM, ptCompare);                                       \
        return MDI_PWM_Commit(PWM);                                              \
    }
#endif
