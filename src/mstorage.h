#ifndef __MSTORAGE_H__
#define __MSTORAGE_H__

#include <stdint.h>
#include "mbase.h"
#include "mdi/legacy/mdi.h"

/**
 * @brief Storage data descriptor
 *
 * ptFlash      — GDI Flash device pointer (chip implements mdi_flash_t)
 * wFlashAddr   — Flash address where data is stored
 * pchStorageStartAddr — RAM buffer pointer (data mirror in RAM)
 * hwStorageLength     — Length of user data (bytes, excluding CRC)
 */
typedef struct {
    mdi_flash_t *ptFlash;           /**< GDI Flash device               */
    uint32_t     wFlashAddr;        /**< Flash storage start address     */
    uint8_t     *pchStorageStartAddr; /**< RAM buffer pointer            */
    uint16_t     hwStorageLength;   /**< User data length (excl. CRC)   */
} mstorage_data_t;

typedef struct {
    mstorage_data_t *ptStorageObject;
    uint16_t         hwStorageTimeOut;
} mstorage_cfg_t;

typedef struct {
    modus_base_t     *ptBase;
    mstorage_data_t *ptStorageObject;
    uint32_t         wTimer;
    uint16_t         hwStorageTimeOut;
    uint16_t         hwLastCrc;
    uint8_t          chStorageFlag;
} mstorage_t;

int mstorage_Init(uintptr_t wObjectAddr, uintptr_t wObjectCfgAddr);

/**
 * @brief 设置模块级默认 Flash 设备。
 *
 * 由 modus_Init 内部调用。当 mstorage_data_t.ptFlash == NULL 时，
 * mstorage_Init 使用此默认值。
 */
void mstorage_SetDefaultFlash(void *ptFlash);


#endif  /* __MSTORAGE_H__ */
