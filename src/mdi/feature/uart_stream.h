/**
 * @file uart_stream.h
 * @brief Static UART byte-stream feature.
 *
 * Queueing and frame timing are shared by all UART resources. A chip backend
 * supplies only register primitives through the binding macro.
 */
#ifndef MDI_FEATURE_UART_STREAM_H
#define MDI_FEATURE_UART_STREAM_H

#include <stdbool.h>
#include <stdint.h>

#include "mdi/core/contract.h"
#include "mdi/core/stream.h"
#include "mringbuf.h"

#define MDI_UART_STREAM_RX_IDLE  0U
#define MDI_UART_STREAM_RX_BUSY  1U
#define MDI_UART_STREAM_RX_READY 2U
#ifndef MDI_UART_STREAM_RX_GUARD_MS
#define MDI_UART_STREAM_RX_GUARD_MS 4U
#endif

typedef struct {
    mringbuf_t tTxQueue;
    mringbuf_t tRxQueue;
    volatile uint8_t chTxBusy;
    volatile uint8_t chRxState;
    volatile uint8_t chRxGuardMs;
} mdi_uart_stream_state_t;

/** @brief Initialize one statically allocated UART stream state. */
MDI_INLINE mdi_status_t mdi_uart_stream_Init(
    mdi_uart_stream_state_t *ptState,
    uint8_t *pchTxBuffer, uint16_t hwTxSize,
    uint8_t *pchRxBuffer, uint16_t hwRxSize)
{
    if (ptState == NULL || pchTxBuffer == NULL || pchRxBuffer == NULL ||
        hwTxSize < 2U || hwRxSize < 2U) {
        return MDI_INVALID;
    }
    if (mringbuf_Init(&ptState->tTxQueue, pchTxBuffer, hwTxSize) < 0 ||
        mringbuf_Init(&ptState->tRxQueue, pchRxBuffer, hwRxSize) < 0) {
        return MDI_RANGE;
    }
    ptState->chTxBusy = 0U;
    ptState->chRxState = MDI_UART_STREAM_RX_IDLE;
    ptState->chRxGuardMs = 0U;
    return MDI_OK;
}

MDI_INLINE int32_t mdi_uart_stream_QueueWrite(
    mdi_uart_stream_state_t *ptState,
    const uint8_t *pchData, uint32_t wLength)
{
    uint32_t wIndex = 0U;

    if (ptState == NULL || (pchData == NULL && wLength != 0U) ||
        wLength > INT32_MAX) {
        return MDI_INVALID;
    }
    while (wIndex < wLength &&
           mringbuf_Write(&ptState->tTxQueue, pchData[wIndex]) != 0U) {
        ++wIndex;
    }
    return (int32_t)wIndex;
}

MDI_INLINE int32_t mdi_uart_stream_QueueRead(
    mdi_uart_stream_state_t *ptState, uint8_t *pchData, uint32_t wLength)
{
    uint32_t wIndex = 0U;

    if (ptState == NULL || (pchData == NULL && wLength != 0U) ||
        wLength > INT32_MAX) {
        return MDI_INVALID;
    }
    if (wLength == 0U || ptState->chRxState != MDI_UART_STREAM_RX_READY) {
        return 0;
    }
    while (wIndex < wLength &&
           mringbuf_Read(&ptState->tRxQueue, &pchData[wIndex]) != 0U) {
        ++wIndex;
    }
    if (mringbuf_GetUsed(&ptState->tRxQueue) == 0U) {
        ptState->chRxState = MDI_UART_STREAM_RX_IDLE;
    }
    return (int32_t)wIndex;
}

MDI_INLINE uint32_t mdi_uart_stream_Available(
    mdi_uart_stream_state_t *ptState)
{
    return ptState == NULL ? 0U :
           (uint32_t)mringbuf_GetUsed(&ptState->tRxQueue);
}

MDI_INLINE bool mdi_uart_stream_IsBusy(mdi_uart_stream_state_t *ptState)
{
    return ptState != NULL && ptState->chTxBusy != 0U;
}

MDI_INLINE void mdi_uart_stream_Tick1ms(
    mdi_uart_stream_state_t *ptState)
{
    if (ptState == NULL || ptState->chRxState != MDI_UART_STREAM_RX_BUSY) {
        return;
    }
    if (ptState->chRxGuardMs > 0U) {
        --ptState->chRxGuardMs;
    } else {
        ptState->chRxState = MDI_UART_STREAM_RX_READY;
    }
}

/**
 * @brief Bind one UART to the generic stream state machine.
 *
 * The primitive arguments are static inline functions or macros with these
 * signatures, where UART is the compile-time hardware expression:
 * RX_READY(UART), RX_READ(UART), TX_COMPLETE(UART), TX_WRITE(UART, byte),
 * TX_CLEAR(UART), TX_IRQ_ENABLE(UART), and TX_IRQ_DISABLE(UART).
 */
#define MDI_UART_STREAM_BIND(                                                  \
    NAME, STATE, UART, RX_READY, RX_READ, TX_COMPLETE, TX_WRITE, TX_CLEAR,     \
    TX_IRQ_ENABLE, TX_IRQ_DISABLE)                                             \
    MDI_INLINE int32_t MDI_OP(NAME, _stream_Write)(                             \
        const uint8_t *pchData, uint32_t wLength)                               \
    {                                                                            \
        int32_t nWritten = mdi_uart_stream_QueueWrite(                          \
            &(STATE), pchData, wLength);                                        \
        if (nWritten >= 0 && (STATE).chTxBusy == 0U &&                         \
            mringbuf_GetUsed(&(STATE).tTxQueue) != 0U) {                        \
            uint8_t chData;                                                     \
            if (mringbuf_Read(&(STATE).tTxQueue, &chData) != 0U) {               \
                TX_CLEAR((UART));                                              \
                TX_WRITE((UART), chData);                                       \
                (STATE).chTxBusy = 1U;                                          \
                TX_IRQ_ENABLE((UART));                                          \
            }                                                                    \
        }                                                                        \
        return nWritten;                                                         \
    }                                                                            \
    MDI_INLINE int32_t MDI_OP(NAME, _stream_Read)(                              \
        uint8_t *pchData, uint32_t wLength)                                     \
    {                                                                            \
        return mdi_uart_stream_QueueRead(&(STATE), pchData, wLength);            \
    }                                                                            \
    MDI_INLINE uint32_t MDI_OP(NAME, _stream_Available)(void)                   \
    {                                                                            \
        return mdi_uart_stream_Available(&(STATE));                             \
    }                                                                            \
    MDI_INLINE bool MDI_OP(NAME, _stream_IsBusy)(void)                          \
    {                                                                            \
        return mdi_uart_stream_IsBusy(&(STATE));                                \
    }                                                                            \
    MDI_INLINE void MDI_OP(NAME, _uart_Tick)(void)                              \
    {                                                                            \
        mdi_uart_stream_Tick1ms(&(STATE));                                      \
    }                                                                            \
    MDI_INLINE void MDI_OP(NAME, _uart_IRQHandler)(void)                        \
    {                                                                            \
        if (RX_READY((UART))) {                                                 \
            (void)mringbuf_Write(&(STATE).tRxQueue, RX_READ((UART)));            \
            (STATE).chRxGuardMs = MDI_UART_STREAM_RX_GUARD_MS;                  \
            (STATE).chRxState = MDI_UART_STREAM_RX_BUSY;                        \
        }                                                                        \
        if (TX_COMPLETE((UART))) {                                              \
            uint8_t chData;                                                     \
            TX_CLEAR((UART));                                                   \
            if (mringbuf_Read(&(STATE).tTxQueue, &chData) != 0U) {               \
                TX_WRITE((UART), chData);                                       \
                (STATE).chTxBusy = 1U;                                          \
            } else {                                                             \
                (STATE).chTxBusy = 0U;                                          \
                TX_IRQ_DISABLE((UART));                                         \
            }                                                                    \
        }                                                                        \
    }

#define MDI_UART_STREAM_TICK_1MS(R) MDI_OP(R, _uart_Tick)()
#define MDI_UART_STREAM_IRQ(R) MDI_OP(R, _uart_IRQHandler)()
#define MDI_UART_STREAM_INIT(STATE, TXBUF, TXSIZE, RXBUF, RXSIZE)                \
    mdi_uart_stream_Init(&(STATE), (TXBUF), (TXSIZE), (RXBUF), (RXSIZE))

#endif /* MDI_FEATURE_UART_STREAM_H */
