#include "mstorage.h"
#include "mlog.h"
#include "mdebug/util_debug.h"
#include <string.h>

int mstorage_Run(uintptr_t wObjectAddr);
int mstorage_Clock(uintptr_t wObjectAddr);

static uint16_t mstorage_CalculateCrc16(uint8_t *pchData, uint16_t hwLen)
{
    uint16_t hwCrc = 0xFFFF;
    for (uint16_t i = 0; i < hwLen; i++) {
        hwCrc ^= (uint16_t)pchData[i];
        for (int j = 0; j < 8; j++) {
            if (hwCrc & 0x0001) {
                hwCrc = (hwCrc >> 1) ^ 0xA001; // CRC-16-Modbus
            } else {
                hwCrc >>= 1;
            }
        }
    }
    return hwCrc;
}

// Define a global storage base of type modus_base_t
static modus_base_t s_tStorageBase;
// Define and initialize a global storage base configuration
modus_base_cfg_t s_tStorageBaseCfg = {
    .wId = MODUS_STORAGE,
    .wParent = 0,
    .FcnInterface = {
        .Clock = mstorage_Clock,
        .Run   = mstorage_Run,
    },
};

/** @brief 模块级默认 Flash 设备，由 modus_Init 通过注入 */
static mdi_flash_t *s_ptDefaultFlash = NULL;

void mstorage_SetDefaultFlash(void *ptFlash)
{
    s_ptDefaultFlash = (mdi_flash_t *)ptFlash;
}

/**
 * @brief Handle storage events.
 */
void mstorage_EventHandle(mstorage_t *ptThis, uint32_t wEvent)
{
    if (ptThis == NULL || ptThis->ptStorageObject == NULL) {
        MLOG(E, "Error: ptThis or ptThis->ptStorageObject is NULL.\n");
        return;
    }

    mstorage_data_t *ptObj  = ptThis->ptStorageObject;
    mdi_flash_t     *ptFlash = ptObj->ptFlash;
    uint32_t         wAddr   = ptObj->wFlashAddr;
    uint16_t         len     = ptObj->hwStorageLength;

    if (ptFlash == NULL) {
        MLOG(E, "MStorage: Flash device is not configured.\n");
        return;
    }

    if (wEvent & Event_Storage)
    {
        uint16_t hwCrc = mstorage_CalculateCrc16(
            ptObj->pchStorageStartAddr, len);

        /* Append CRC at the end (2 bytes, little-endian) */
        ptObj->pchStorageStartAddr[len]     = (uint8_t)(hwCrc & 0xFF);
        ptObj->pchStorageStartAddr[len + 1] = (uint8_t)((hwCrc >> 8) & 0xFF);

        MLOGF(I, "MStorage: Saving data, CRC: 0x%04X\n", hwCrc);

        int32_t nUnlock = mdi_flash_Unlock(ptFlash);
        int32_t nErase = -1;
        int32_t nRet = -1;
        int32_t nLock = -1;

        if (nUnlock >= 0) {
            nErase = mdi_flash_Erase(
                ptFlash, wAddr, (uint32_t)(len + 2));
            if (nErase >= 0) {
                nRet = mdi_flash_Write(
                    ptFlash, wAddr, ptObj->pchStorageStartAddr,
                    (uint32_t)(len + 2));
            }
            nLock = mdi_flash_Lock(ptFlash);
        }

        if (nUnlock >= 0 && nErase >= 0 && nRet >= 0 && nLock >= 0) {
            ptThis->hwLastCrc = hwCrc;
        } else {
            MLOGF(E, "MStorage: Write Failed (%d/%d/%d/%d).\n",
                  (int)nUnlock, (int)nErase, (int)nRet, (int)nLock);
        }
    }

    if (wEvent & Event_ResetDefault)
    {
        MLOG(W, "MStorage: Blanking Flash (RAM untouched)...\n");
        /* 只清空 Flash，不修改 RAM。
         * 系统继续以当前 RAM 数据运行，硬件行为不受影响。
         * 下次上电时 mstorage_Init 检测到 Flash 全 FF，
         * 会将编译期静态默认值保存到 Flash 并恢复。 */
        int32_t nUnlock = mdi_flash_Unlock(ptFlash);
        int32_t nErase = -1;
        int32_t nLock = -1;

        if (nUnlock >= 0) {
            nErase = mdi_flash_Erase(
                ptFlash, wAddr, (uint32_t)(len + 2));
            nLock = mdi_flash_Lock(ptFlash);
        }
        if (nUnlock < 0 || nErase < 0 || nLock < 0) {
            MLOGF(E, "MStorage: Blank Failed (%d/%d/%d).\n",
                  (int)nUnlock, (int)nErase, (int)nLock);
        }

        /* 同步 hwLastCrc 到当前 RAM 的 CRC，
         * 防止 Clock 检测到"CRC 变化"立即把当前 RAM 重写回 Flash。 */
        ptThis->hwLastCrc = mstorage_CalculateCrc16(
            ptObj->pchStorageStartAddr, len);
        MLOG(I, "MStorage: Flash blanked. Reboot to apply defaults.\n");
    }

    if (wEvent & Event_GetData)
    {
        MLOG(I, "MStorage: Loading data...\n");
        int32_t nRet = mdi_flash_Read(ptFlash, wAddr, 
                                      ptObj->pchStorageStartAddr, 
                                      (uint32_t)(len + 2));

        if (nRet >= 0) {
            uint16_t hwReadCrc = ptObj->pchStorageStartAddr[len];
            hwReadCrc |= (uint16_t)(ptObj->pchStorageStartAddr[len + 1] << 8);

            uint16_t hwCalcCrc = mstorage_CalculateCrc16(
                ptObj->pchStorageStartAddr, len);

            if (hwReadCrc == hwCalcCrc) {
                ptThis->hwLastCrc = hwReadCrc;
                MLOG(I, "MStorage: Load Success, CRC Match.\n");
            } else {
                MLOGF(E, "MStorage: CRC Mismatch! Read: 0x%04X, Calc: 0x%04X\n", 
                      hwReadCrc, hwCalcCrc);
            }
        } else {
            MLOGF(E, "MStorage: Read Failed (%d).\n", (int)nRet);
        }
    }
}

/**
 * @brief Run the storage object.
 */
int mstorage_Run(uintptr_t wObjectAddr)
{
    mstorage_t *ptThis = (mstorage_t *)wObjectAddr;

    if (ptThis == NULL || ptThis->ptBase == NULL) {
        MLOG(E, "Error: ptThis or ptThis->ptBase is NULL.\n");
        return MODUS_EFAIL;
    }

    uint32_t wEvent = mbase_EventPend(ptThis->ptBase);
    if (wEvent) {
        mstorage_EventHandle(ptThis, wEvent);
    }

    return MODUS_SUCCESS;
}

/**
 * @brief Clock task for storage object.
 */
int mstorage_Clock(uintptr_t wObjectAddr)
{
    mstorage_t *ptThis = (mstorage_t *)wObjectAddr;

    if (ptThis == NULL || ptThis->ptStorageObject == NULL) {
        return MODUS_EFAIL;
    }

    if (ptThis->hwStorageTimeOut == 0) {
        return MODUS_SUCCESS;
    }

    if (ptThis->wTimer > 0) {
        ptThis->wTimer--;
    } else {
        ptThis->wTimer = ptThis->hwStorageTimeOut;

        uint16_t hwCurrentCrc = mstorage_CalculateCrc16(
            ptThis->ptStorageObject->pchStorageStartAddr,
            ptThis->ptStorageObject->hwStorageLength
        );

        if (hwCurrentCrc != ptThis->hwLastCrc) {
            mbase_EventPost(ptThis->ptBase->wId, Event_Storage);
        }
    }

    return MODUS_SUCCESS;
}

/**
 * @brief Initialize the storage object.
 */
int mstorage_Init(uintptr_t wObjectAddr, uintptr_t wObjectCfgAddr)
{
    mstorage_t     *ptThis = (mstorage_t *)wObjectAddr;
    mstorage_cfg_t *ptCfg  = (mstorage_cfg_t *)wObjectCfgAddr;

    if (ptThis == NULL || ptCfg == NULL) {
        MLOG(E, "Error: ptThis or ptCfg is NULL.\n");
        return MODUS_EFAIL;
    }

    ptThis->ptStorageObject  = ptCfg->ptStorageObject;
    ptThis->ptBase           = &s_tStorageBase;
    ptThis->hwStorageTimeOut = ptCfg->hwStorageTimeOut;
    ptThis->wTimer           = ptCfg->hwStorageTimeOut;

    s_tStorageBaseCfg.wParent = wObjectAddr;
    int wRet = mbase_Init(ptThis->ptBase, &s_tStorageBaseCfg);
    if (wRet < 0) {
        MLOGF(E, "mbase_Init failed: %d\n", wRet);
        return wRet;
    }

    mstorage_data_t *ptObj  = ptThis->ptStorageObject;

    /* 若配置时 ptFlash 未填，使用 modus_Init 注入的全局默认值 */
    if (ptObj != NULL && ptObj->ptFlash == NULL) {
        ptObj->ptFlash = s_ptDefaultFlash;
    }

    mdi_flash_t     *ptFlash = ptObj ? ptObj->ptFlash : NULL;

    if (ptObj == NULL || ptFlash == NULL) {
        return MODUS_SUCCESS; /* No flash configured, skip init read */
    }

    uint32_t wAddr = ptObj->wFlashAddr;
    uint16_t len   = ptObj->hwStorageLength;

    /* Initial read from Flash */
    int32_t nRead = mdi_flash_Read(
        ptFlash, wAddr, ptObj->pchStorageStartAddr,
        (uint32_t)(len + 2));
    if (nRead < 0) {
        MLOGF(E, "MStorage: Initial read failed (%d).\n", (int)nRead);
        return MODUS_EFAIL;
    }

    bool bIsBlank = true;
    for (uint16_t i = 0; i < len + 2; i++) {
        if (ptObj->pchStorageStartAddr[i] != 0xFF) {
            bIsBlank = false;
            break;
        }
    }

    if (bIsBlank) {
        MLOG(W, "MStorage: Storage blank. Initializing defaults...\n");
        mbase_EventPost(ptThis->ptBase->wId, Event_Storage);
    } else {
        uint16_t hwReadCrc = ptObj->pchStorageStartAddr[len];
        hwReadCrc |= (uint16_t)(ptObj->pchStorageStartAddr[len + 1] << 8);

        uint16_t hwCalcCrc = mstorage_CalculateCrc16(
            ptObj->pchStorageStartAddr, len);

        if (hwReadCrc == hwCalcCrc) {
            ptThis->hwLastCrc = hwReadCrc;
            MLOGF(I, "MStorage: Load Success, CRC: 0x%04X\n", hwReadCrc);
        } else {
            MLOGF(E, "MStorage: CRC Error! Calc: 0x%04X, Stored: 0x%04X\n", 
                  hwCalcCrc, hwReadCrc);
        }
    }

    return MODUS_SUCCESS;
}
