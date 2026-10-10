/**
 * @file uart_stream.h
 * @brief Static UART queues with immediate reads and bytewise transmission.
 * @author Codex
 * @date 2026-10-10
 */
#ifndef MDI_FEATURE_UART_STREAM_H
#define MDI_FEATURE_UART_STREAM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>

#include "mdi/core/contract.h"
#include "mdi/core/stream.h"
#include "mringbuf.h"

/* One foreground producer/consumer, one non-nested UART ISR. RX head belongs
 * to the ISR; RX tail and TX head belong to foreground. TX tail belongs to
 * the ISR while TCIE is enabled, otherwise foreground may prime one byte.
 * Initialization and failure handling require this UART's IRQ to be disabled.
 * The ISR sets chRxOverrun; a nonzero Read acknowledges a set flag and reports
 * MDI_OVERRUN with any bytes read. Losses before acknowledgement coalesce.
 */
typedef struct {
    mringbuf_t tTxQueue;
    mringbuf_t tRxQueue;
    mdi_status_t eInitStatus;
    volatile uint8_t chTxBusy;
    atomic_uchar chRxOverrun;
} mdi_uart_stream_state_t;

/**
 * @brief Return initialization failure or readiness.
 * @param ptState Stream state, initialized before enabling the UART IRQ.
 * @return OK, retained initialization failure, or IO_ERROR before Init.
 */
MDI_INLINE mdi_status_t mdi_uart_stream_Status(
    const mdi_uart_stream_state_t *ptState)
{
    if (ptState == NULL) {
        return MDI_INVALID;
    }
    if (ptState->eInitStatus != MDI_OK) {
        return ptState->eInitStatus;
    }
    return ptState->tTxQueue.pchBuffer == NULL ? MDI_IO_ERROR : MDI_OK;
}

/**
 * @brief Retain a board initialization failure before its IRQ is enabled.
 * @param ptState Board-owned stream state.
 * @param eStatus Failure returned by board setup.
 * @return None.
 */
MDI_INLINE void mdi_uart_stream_FailInit(
    mdi_uart_stream_state_t *ptState, mdi_status_t eStatus)
{
    if (ptState != NULL && eStatus != MDI_OK) {
        ptState->eInitStatus = eStatus;
    }
}

/**
 * @brief Initialize one statically allocated UART stream state.
 * @param ptState Board-owned state; its UART IRQ must be disabled.
 * @param pchTxBuffer TX storage. @param hwTxSize Power-of-two storage size.
 * @param pchRxBuffer RX storage. @param hwRxSize Power-of-two storage size.
 * @return OK or retained INVALID/RANGE failure.
 * @note Five arguments retain the existing two-buffer ownership contract.
 */
MDI_INLINE mdi_status_t mdi_uart_stream_Init(
    mdi_uart_stream_state_t *ptState,
    uint8_t *pchTxBuffer, uint16_t hwTxSize,
    uint8_t *pchRxBuffer, uint16_t hwRxSize)
{
    if (ptState == NULL) {
        return MDI_INVALID;
    }
    ptState->eInitStatus = MDI_INVALID;
    ptState->chTxBusy = 0U;
    atomic_init(&ptState->chRxOverrun, 0U);
    if (pchTxBuffer == NULL || pchRxBuffer == NULL ||
        hwTxSize < 2U || hwRxSize < 2U) {
        return ptState->eInitStatus;
    }
    if (mringbuf_Init(&ptState->tTxQueue, pchTxBuffer, hwTxSize) < 0 ||
        mringbuf_Init(&ptState->tRxQueue, pchRxBuffer, hwRxSize) < 0) {
        ptState->eInitStatus = MDI_RANGE;
        return ptState->eInitStatus;
    }
    ptState->eInitStatus = MDI_OK;
    return MDI_OK;
}

/**
 * @brief Enqueue as many bytes as fit without waiting.
 * @param ptState Stream state. @param pchData Borrowed source bytes.
 * @param wLength Requested count. @param pwCount Actual enqueued count.
 * @return OK (including short writes), INVALID, or initialization failure.
 */
MDI_INLINE mdi_status_t mdi_uart_stream_QueueWrite(
    mdi_uart_stream_state_t *ptState,
    const uint8_t *pchData, uint32_t wLength, uint32_t *pwCount)
{
    mdi_status_t eStatus = mdi_uart_stream_Status(ptState);
    if (pwCount == NULL || (pchData == NULL && wLength != 0U)) {
        return MDI_INVALID;
    }
    *pwCount = 0U;
    if (eStatus != MDI_OK) {
        return eStatus;
    }
    while (*pwCount < wLength &&
           mringbuf_Write(&ptState->tTxQueue, pchData[*pwCount]) != 0U) {
        ++*pwCount;
    }
    return MDI_OK;
}

/**
 * @brief Read queued bytes immediately, acknowledging any observed RX loss.
 * @param ptState Stream state. @param pchData Destination bytes.
 * @param wLength Requested count. @param pwCount Actual copied count.
 * @return OK, OVERRUN with valid copied bytes, or argument/init failure.
 */
MDI_INLINE mdi_status_t mdi_uart_stream_QueueRead(
    mdi_uart_stream_state_t *ptState, uint8_t *pchData,
    uint32_t wLength, uint32_t *pwCount)
{
    mdi_status_t eStatus = mdi_uart_stream_Status(ptState);
    if (pwCount == NULL || (pchData == NULL && wLength != 0U)) {
        return MDI_INVALID;
    }
    *pwCount = 0U;
    if (eStatus != MDI_OK || wLength == 0U) {
        return eStatus;
    }
    while (*pwCount < wLength &&
           mringbuf_Read(&ptState->tRxQueue, &pchData[*pwCount]) != 0U) {
        ++*pwCount;
    }
    if (atomic_exchange_explicit(&ptState->chRxOverrun, 0U,
                                 memory_order_relaxed) != 0U) {
        return MDI_OVERRUN;
    }
    return MDI_OK;
}

/** @brief Query immediately readable bytes. @param ptState Stream state.
 * @return Queued count, or zero when unready.
 */
MDI_INLINE uint32_t mdi_uart_stream_Available(
    mdi_uart_stream_state_t *ptState)
{
    return mdi_uart_stream_Status(ptState) == MDI_OK ?
           (uint32_t)mringbuf_GetUsed(&ptState->tRxQueue) : 0U;
}

/** @brief Include queued data and the physically transmitting last byte.
 * @param ptState Stream state. @return True until the final TC interrupt.
 */
MDI_INLINE bool mdi_uart_stream_IsBusy(mdi_uart_stream_state_t *ptState)
{
    return mdi_uart_stream_Status(ptState) == MDI_OK &&
           (ptState->chTxBusy != 0U ||
            mringbuf_GetUsed(&ptState->tTxQueue) != 0U);
}

/**
 * @brief Bind direct register primitives to one foreground/ISR stream.
 * @note RX_LOST acknowledges hardware overrun. TX_COMPLETE must require both
 * the physical TC flag and enabled TC interrupt: RX IRQs may occur while
 * foreground primes the idle transmitter, whose TC flag can still be set.
 */
#define MDI_UART_STREAM_BIND(                                                   \
    NAME, STATE, UART, RX_READY, RX_READ, RX_LOST, TX_COMPLETE, TX_WRITE,       \
    TX_CLEAR, TX_IRQ_ENABLE, TX_IRQ_DISABLE)                                    \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _stream_Write)(                        \
        const uint8_t *pchData, uint32_t wLength, uint32_t *pwCount)            \
    {                                                                           \
        mdi_status_t eStatus = mdi_uart_stream_QueueWrite(                      \
            &(STATE), pchData, wLength, pwCount);                               \
        if (eStatus == MDI_OK && (STATE).chTxBusy == 0U &&                      \
            mringbuf_GetUsed(&(STATE).tTxQueue) != 0U) {                        \
            uint8_t chData = 0U;                                                \
            if (mringbuf_Read(&(STATE).tTxQueue, &chData) != 0U) {              \
                TX_CLEAR((UART));                                               \
                TX_WRITE((UART), chData);                                       \
                (STATE).chTxBusy = 1U;                                          \
                TX_IRQ_ENABLE((UART));                                          \
            }                                                                   \
        }                                                                       \
        return eStatus;                                                         \
    }                                                                           \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _stream_Read)(                         \
        uint8_t *pchData, uint32_t wLength, uint32_t *pwCount)                  \
    {                                                                           \
        return mdi_uart_stream_QueueRead(                                       \
            &(STATE), pchData, wLength, pwCount);                               \
    }                                                                           \
    MDI_INLINE uint32_t MDI_OP(NAME, _stream_Available)(void)                   \
    {                                                                           \
        return mdi_uart_stream_Available(&(STATE));                             \
    }                                                                           \
    MDI_INLINE bool MDI_OP(NAME, _stream_IsBusy)(void)                          \
    {                                                                           \
        return mdi_uart_stream_IsBusy(&(STATE));                                \
    }                                                                           \
    MDI_INLINE void MDI_OP(NAME, _uart_IRQHandler)(void)                        \
    {                                                                           \
        if (mdi_uart_stream_Status(&(STATE)) != MDI_OK) {                       \
            return;                                                             \
        }                                                                       \
        if (RX_LOST((UART))) {                                                  \
            atomic_store_explicit(&(STATE).chRxOverrun, 1U,                     \
                                  memory_order_relaxed);                        \
        }                                                                       \
        if (RX_READY((UART)) &&                                                 \
            mringbuf_Write(&(STATE).tRxQueue, RX_READ((UART))) == 0U) {         \
            atomic_store_explicit(&(STATE).chRxOverrun, 1U,                     \
                                  memory_order_relaxed);                        \
        }                                                                       \
        if (TX_COMPLETE((UART))) {                                              \
            uint8_t chData = 0U;                                                \
            TX_CLEAR((UART));                                                   \
            if (mringbuf_Read(&(STATE).tTxQueue, &chData) != 0U) {              \
                TX_WRITE((UART), chData);                                       \
            } else {                                                            \
                (STATE).chTxBusy = 0U;                                          \
                TX_IRQ_DISABLE((UART));                                         \
            }                                                                   \
        }                                                                       \
    }

#define MDI_UART_STREAM_IRQ(R) MDI_OP(R, _uart_IRQHandler)()
#define MDI_UART_STREAM_INIT(STATE, TXBUF, TXSIZE, RXBUF, RXSIZE)               \
    mdi_uart_stream_Init(&(STATE), (TXBUF), (TXSIZE), (RXBUF), (RXSIZE))

#endif /* MDI_FEATURE_UART_STREAM_H */
