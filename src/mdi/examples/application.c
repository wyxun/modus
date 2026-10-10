/**
 * @file application.c
 * @brief Review entry point: application calls contain no hardware symbols.
 * @author Codex
 * @date 2026-09-18
 */
#include "mdi/instance.h"

/** @brief Drive the board's logical status indicator.
 * @param bOn True lights the indicator.
 * @return None.
 */
void example_WriteOutputs(bool bOn)
{
    MDI_IO_Write(status_led, bOn ? 1U : 0U);
}

/** @brief Consume one injected ADC frame only after JEOS publication.
 * @param ptCurrent Caller-owned output frame.
 * @return MDI_OK, MDI_BUSY while conversion is incomplete, or invalid.
 */
mdi_status_t example_ReadCompletedCurrent(
    MDI_Sample_Frame(phase_current_completed) *ptCurrent)
{
    return MDI_Sample_ReadCompleted(phase_current_completed, ptCurrent);
}

/** @brief Consume one completed current frame and submit one PWM duty frame.
 * @param ptCurrent Caller-owned current frame.
 * @param ptDuty Normalized Q16 duty frame.
 * @return Busy until JEOS is published, or the first provider error.
 */
mdi_status_t example_FocCycle(
    MDI_Sample_Frame(phase_current_completed) *ptCurrent,
    const MDI_PWM_DutyFrame(bridge) *ptDuty)
{
    mdi_status_t eStatus = MDI_Sample_ReadCompleted(
        phase_current_completed, ptCurrent);
    if (eStatus != MDI_OK) {
        return eStatus;
    }
    return MDI_PWM_SetDuty(bridge, ptDuty);
}

/** @brief Change a stopped, output-gated buzzer's frequency and duty.
 * @param wHz Requested frequency, rounded to timer resolution.
 * @return Configuration status; failed frequency leaves the timer untouched.
 * @note The board owns initialization and subsequent enabling of outputs.
 */
mdi_status_t example_ConfigureBuzzer(uint32_t wHz)
{
    const MDI_PWM_DutyFrame(buzzer) tDuty = {.value = 32768U};
    mdi_status_t eStatus = MDI_PWM_SetFrequency(buzzer, wHz);
    if (eStatus != MDI_OK) {
        return eStatus;
    }
    return MDI_PWM_SetDuty(buzzer, &tDuty);
}

/** @brief Submit a three-channel normalized duty command.
 * @param ptDuty Q16 fractions in 0..65536, independent of timer frequency.
 * @return Status; no compare is changed on a range error.
 */
mdi_status_t example_StageDuty(const MDI_PWM_DutyFrame(bridge) *ptDuty)
{
    return MDI_PWM_SetDuty(bridge, ptDuty);
}

/** @brief Submit prevalidated timer ticks in a bounded real-time path.
 * @param ptTicks Nonnull tick frame, each value at most current period.
 * @return None.
 * @note The caller owns the group and guarantees the preload update deadline.
 */
void example_StageTicks(const MDI_PWM_Frame(bridge) *ptTicks)
{
    MDI_PWM_StageFast(bridge, ptTicks);
}
