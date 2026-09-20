/**
 * @file    mdi_static.h
 * @brief   Compile-time MDI capability dispatch helpers.
 * @author  Codex
 * @date    2026-09-17
 */

#ifndef MDI_STATIC_H
#define MDI_STATIC_H

#include <stdint.h>

#ifndef MDI_LEGACY_STATUS_T
#define MDI_LEGACY_STATUS_T
typedef int32_t mdi_legacy_status_t;
#endif

#define MDI_STATUS_OK        ((mdi_legacy_status_t)0)
#define MDI_STATUS_EINVAL    ((mdi_legacy_status_t)-1)
#define MDI_STATUS_ENODEV    ((mdi_legacy_status_t)-2)
#define MDI_STATUS_EBUSY     ((mdi_legacy_status_t)-3)
#define MDI_STATUS_ETIMEOUT  ((mdi_legacy_status_t)-4)
#define MDI_STATUS_EIO       ((mdi_legacy_status_t)-5)
#define MDI_STATUS_ENOTSUP   ((mdi_legacy_status_t)-6)
#define MDI_STATUS_EAGAIN    ((mdi_legacy_status_t)-7)

/*
 * Each board binding defines the _Generic association list before including
 * this header. The selected operation is a direct or static inline function.
 */
#define MDI_PWM_SetDuty(ptDev, wDuty)                                            \
    _Generic((ptDev), MDI_PWM_SET_DUTY_ASSOCIATIONS)((ptDev), (wDuty))

#define MDI_PWM_SetFrequency(ptDev, wFrequencyHz)                                \
    _Generic((ptDev), MDI_PWM_SET_FREQUENCY_ASSOCIATIONS)(                       \
        (ptDev), (wFrequencyHz))

#define MDI_PWM_SetDuty3(ptDev, wDutyU, wDutyV, wDutyW)                          \
    _Generic((ptDev), MDI_PWM_SET_DUTY3_ASSOCIATIONS)(                           \
        (ptDev), (wDutyU), (wDutyV), (wDutyW))

#define MDI_PWM_Enable(ptDev, bEnable)                                           \
    _Generic((ptDev), MDI_PWM_ENABLE_ASSOCIATIONS)((ptDev), (bEnable))

#define MDI_PWM_SafeStop(ptDev)                                                  \
    _Generic((ptDev), MDI_PWM_SAFE_STOP_ASSOCIATIONS)((ptDev))

#define MDI_GPIO_Set(ptDev, eLevel)                                              \
    _Generic((ptDev), MDI_GPIO_SET_ASSOCIATIONS)((ptDev), (eLevel))

#define MDI_GPIO_Get(ptDev, peLevel)                                             \
    _Generic((ptDev), MDI_GPIO_GET_ASSOCIATIONS)((ptDev), (peLevel))

#define MDI_GPIO_Toggle(ptDev)                                                   \
    _Generic((ptDev), MDI_GPIO_TOGGLE_ASSOCIATIONS)((ptDev))

#define MDI_ADC_Sample(ptDev, pwSample)                                          \
    _Generic((ptDev), MDI_ADC_SAMPLE_ASSOCIATIONS)((ptDev), (pwSample))

#define MDI_ADC_SamplePhaseCurrent(                                              \
    ptDev, pwSampleU, pwSampleV, pwSampleW)                                      \
    _Generic((ptDev), MDI_ADC_SAMPLE_PHASE_CURRENT_ASSOCIATIONS)(                \
        (ptDev), (pwSampleU), (pwSampleV), (pwSampleW))

/* Stream keeps Read/Write semantics because a byte stream is a data path. */
#define MDI_Stream_Write(ptDev, pchData, wLen)                                   \
    _Generic((ptDev), MDI_STREAM_WRITE_ASSOCIATIONS)(                            \
        (ptDev), (pchData), (wLen))

#define MDI_Stream_Read(ptDev, pchBuf, wLen)                                     \
    _Generic((ptDev), MDI_STREAM_READ_ASSOCIATIONS)(                             \
        (ptDev), (pchBuf), (wLen))

#define MDI_Stream_IsBusy(ptDev)                                                 \
    _Generic((ptDev), MDI_STREAM_IS_BUSY_ASSOCIATIONS)((ptDev))

#endif /* MDI_STATIC_H */
