/**
 * @file spi_eeprom_25xx.h
 * @brief Static 25xx SPI EEPROM device transactions.
 * @author Codex
 * @date 2026-09-18
 * @note The SPI resource performs one full-duplex transfer while this device
 * binding owns chip-select framing, page splitting and write-ready polling.
 * The chip-select resource is logical active-low; bind an inverted IO resource
 * for boards using active-high selection.
 */
#ifndef MDI_FEATURE_SPI_EEPROM_25XX_H
#define MDI_FEATURE_SPI_EEPROM_25XX_H
#include "mdi/core/contract.h"

#define MDI_SPI_EEPROM_Read(R, A, P, L)                                          \
    MDI_OP(R, _spi_eeprom_Read)((A), (P), (L))
#define MDI_SPI_EEPROM_Write(R, A, P, L)                                         \
    MDI_OP(R, _spi_eeprom_Write)((A), (P), (L))
#define MDI_SPI_EEPROM_WaitReady(R, N)                                           \
    MDI_OP(R, _spi_eeprom_WaitReady)((N))

/** @brief Bind a 16-bit-address 25xx EEPROM to synchronous SPI and CS.
 * @param NAME EEPROM resource token.
 * @param SPI Synchronous full-duplex SPI resource token.
 * @param CS Active-low logical chip-select IO resource token.
 * @param CAPACITY Total bytes in the device.
 * @param PAGE_SIZE Page-program size in bytes.
 * @param READY_POLLS Default status polls after each page program.
 * @return Generates read/write/wait-ready operations.
 * @note Write splits at page boundaries and waits for WIP to clear after
 * every page. A transfer owns its borrowed buffers only for the call.
 */
#define MDI_SPI_EEPROM_BIND(NAME, SPI, CS, CAPACITY, PAGE_SIZE, READY_POLLS)     \
    _Static_assert((CAPACITY) > 0U && (CAPACITY) <= 65536U,                      \
                   "invalid EEPROM capacity");                                   \
    _Static_assert((PAGE_SIZE) > 0U && (PAGE_SIZE) <= 255U,                      \
                   "invalid EEPROM page size");                                  \
    _Static_assert((READY_POLLS) > 0U, "EEPROM ready poll bound must be          \
    nonzero");                                                                   \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _spi_eeprom_Transfer)(                  \
        const uint8_t *pchTx, uint8_t *pchRx, uint32_t wLength)                  \
    {                                                                            \
        const mdi_spi_transfer_t tTransfer = {                                   \
            .pchTx = pchTx, .pchRx = pchRx, .wLength = wLength,                  \
            .wTimeoutUs = 0U, .chFill = 0xFFU                                    \
        };                                                                       \
        mdi_status_t eStatus;                                                    \
        MDI_IO_Write(CS, 0U);                                                    \
        eStatus = MDI_SPI_Transfer(SPI, &tTransfer);                             \
        MDI_IO_Write(CS, 1U);                                                    \
        return eStatus;                                                          \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _spi_eeprom_WriteEnable)(void)          \
    {                                                                            \
        const uint8_t chCommand = 0x06U;                                         \
        return MDI_OP(NAME, _spi_eeprom_Transfer)(&chCommand, NULL, 1U);         \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _spi_eeprom_WaitReady)(                 \
        uint32_t wPolls)                                                         \
    {                                                                            \
        uint8_t chCommand = 0x05U;                                               \
        uint8_t chStatus = 0xFFU;                                                \
        uint32_t wPoll;                                                          \
        mdi_status_t eStatus;                                                    \
        if (wPolls == 0U) { return MDI_TIMEOUT; }                                \
        for (wPoll = 0U; wPoll < wPolls; ++wPoll) {                              \
            chStatus = 0xFFU;                                                    \
            eStatus = MDI_OP(NAME, _spi_eeprom_Transfer)(                        \
                &chCommand, &chStatus, 1U);                                      \
            if (eStatus != MDI_OK) { return eStatus; }                           \
            if ((chStatus & 0x01U) == 0U) { return MDI_OK; }                     \
        }                                                                        \
        return MDI_TIMEOUT;                                                      \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _spi_eeprom_Write)(                     \
        uint32_t wAddress, const uint8_t *pchData, uint32_t wLength)             \
    {                                                                            \
        uint8_t chFrame[3U + (PAGE_SIZE)];                                       \
        uint32_t wDone = 0U;                                                     \
        mdi_status_t eStatus;                                                    \
        if (wAddress > (CAPACITY) || wLength > ((CAPACITY) - wAddress)) {        \
            return MDI_RANGE;                                                    \
        }                                                                        \
        if (wLength != 0U && pchData == NULL) { return MDI_INVALID; }            \
        while (wDone < wLength) {                                                \
            uint32_t wPageOffset = (wAddress + wDone) % (PAGE_SIZE);             \
            uint32_t wChunk = (PAGE_SIZE) - wPageOffset;                         \
            uint32_t wIndex;                                                     \
            if (wChunk > (wLength - wDone)) { wChunk = wLength - wDone; }        \
            eStatus = MDI_OP(NAME, _spi_eeprom_WriteEnable)();                   \
            if (eStatus != MDI_OK) { return eStatus; }                           \
            chFrame[0] = 0x02U;                                                  \
            chFrame[1] = (uint8_t)((wAddress + wDone) >> 8U);                    \
            chFrame[2] = (uint8_t)(wAddress + wDone);                            \
            for (wIndex = 0U; wIndex < wChunk; ++wIndex) {                       \
                chFrame[3U + wIndex] = pchData[wDone + wIndex];                  \
            }                                                                    \
            eStatus = MDI_OP(NAME, _spi_eeprom_Transfer)(                        \
                chFrame, NULL, 3U + wChunk);                                     \
            if (eStatus != MDI_OK) { return eStatus; }                           \
            eStatus = MDI_OP(NAME, _spi_eeprom_WaitReady)(READY_POLLS);          \
            if (eStatus != MDI_OK) { return eStatus; }                           \
            wDone += wChunk;                                                     \
        }                                                                        \
        return MDI_OK;                                                           \
    }                                                                            \
    MDI_INLINE mdi_status_t MDI_OP(NAME, _spi_eeprom_Read)(                      \
        uint32_t wAddress, uint8_t *pchData, uint32_t wLength)                   \
    {                                                                            \
        uint8_t chTx[3U + (PAGE_SIZE)];                                          \
        uint8_t chRx[3U + (PAGE_SIZE)];                                          \
        uint32_t wDone = 0U;                                                     \
        mdi_status_t eStatus;                                                    \
        if (wAddress > (CAPACITY) || wLength > ((CAPACITY) - wAddress)) {        \
            return MDI_RANGE;                                                    \
        }                                                                        \
        if (wLength != 0U && pchData == NULL) { return MDI_INVALID; }            \
        while (wDone < wLength) {                                                \
            uint32_t wChunk = (PAGE_SIZE);                                       \
            uint32_t wIndex;                                                     \
            if (wChunk > (wLength - wDone)) { wChunk = wLength - wDone; }        \
            chTx[0] = 0x03U;                                                     \
            chTx[1] = (uint8_t)((wAddress + wDone) >> 8U);                       \
            chTx[2] = (uint8_t)(wAddress + wDone);                               \
            for (wIndex = 0U; wIndex < wChunk; ++wIndex) { chTx[3U + wIndex] =   \
    0xFFU; }                                                                     \
            eStatus = MDI_OP(NAME, _spi_eeprom_Transfer)(                        \
                chTx, chRx, 3U + wChunk);                                        \
            if (eStatus != MDI_OK) { return eStatus; }                           \
            for (wIndex = 0U; wIndex < wChunk; ++wIndex) {                       \
                pchData[wDone + wIndex] = chRx[3U + wIndex];                     \
            }                                                                    \
            wDone += wChunk;                                                     \
        }                                                                        \
        return MDI_OK;                                                           \
    }
#endif
