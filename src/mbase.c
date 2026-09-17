#include "global_define.h"
#include "utilities/mlist.h"
#include "mdebug/util_debug.h"
#include "mbase.h"
#include <string.h>

#ifdef LINUX_POSIX
#include <stdio.h>
#include <assert.h>
#endif

static mlist_t tListObject = MLIST_STATIC_INIT(tListObject);

int mbase_ShareMemInit(modus_base_t *ptBase, share_mem_t *ptShareMem);

/**
 * Function: mbase_Init
 * ----------------------------
 * This function initializes a modus_base_t structure. It sets the structure's ID and event, 
 * initializes its list item, and inserts the item into a list. If the parent of the configuration 
 * structure is not zero, it sets the parent of the base structure; otherwise, it returns MODUS_EAGAIN.
 *
 * Parameters: 
 * ptBase: A pointer to the modus_base_t structure to initialize.
 * ptCfg: A pointer to the configuration structure for the base structure.
 *
 * Returns: 
 * A status code indicating the result of the function. MODUS_SUCCESS if the function succeeds, 
 * MODUS_EINVAL if the parent of the configuration structure is zero.
 */
int mbase_Init(modus_base_t *ptBase, modus_base_cfg_t *ptCfg)
{
    int wRet = MODUS_SUCCESS;

    // Check for null pointers
    if (NULL == ptBase || NULL == ptCfg) {
        return MODUS_EINVAL;
    }

    // Set base ID and clear event
    ptBase->wId = ptCfg->wId;
    ptBase->wEvent = 0;

    // Automatically bind RingBuffer if provided
    if (ptCfg->pchRingBuffer != NULL && ptCfg->hwRingSize > 0) {
        ptBase->tRingBuffer.buffer       = ptCfg->pchRingBuffer;
        ptBase->tRingBuffer.hwBufferSize = ptCfg->hwRingSize;
        ptBase->tRingBuffer.hwWriteIndex = 0;
        ptBase->tRingBuffer.hwReadIndex  = 0;
    }

    // Initialise list item and set its value and owner
    mlist_ItemInit(&ptBase->tListItem);
    ptBase->tListItem.wItemValue = ptBase->wId;
    ptBase->tListItem.pvOwner = ptBase;

    // Initialise message list
    mlist_Init(&ptBase->tListMessage);
    // Insert item into list
    mlist_Insert(&tListObject, &ptBase->tListItem);

    // Set parent and function interface
    if(ptCfg->wParent)
        ptBase->wParent = ptCfg->wParent;
    else
        wRet = MODUS_EAGAIN;
    ptBase->pFcnInterface = &ptCfg->FcnInterface;
    // Initialize share memory if provided
    if(NULL != ptCfg->ptShareMem)
    {
        wRet = mbase_ShareMemInit(ptBase, ptCfg->ptShareMem);
        if (wRet != MODUS_SUCCESS) {
            return wRet;
        }
    }
    return wRet;
}

/**
 * Function: mbase_EventPost
 * ----------------------------
 * This function posts an event to a base object with a specific ID. It traverses a list of base objects, 
 * finds the one with the given ID, and updates its event. If no base object with the given ID is found, 
 * it returns an error code.
 *
 * Parameters: 
 * wId: The ID of the base object to post the event to.
 * wEvent: The event to post.
 *
 * Returns: 
 * A status code indicating the result of the function. MODUS_SUCCESS if the function succeeds, 
 * MODUS_ENODEV if no base object with the given ID is found.
 */
int mbase_EventPost(uint32_t wId, uint32_t wEvent)
{
    // Check for valid input
    if (wId == 0 || wEvent == 0) {
        return MODUS_EINVAL;
    }

    int wRet = MODUS_SUCCESS;
    mlist_item_t *ptListItemDes = tListObject.tListEnd.ptPrevious;
    modus_base_t *ptBaseDes;

    // Find the list item with the given ID
    for (; ptListItemDes != &tListObject.tListEnd; ptListItemDes = ptListItemDes->ptPrevious) {
        if(ptListItemDes->wItemValue == wId)
            break;
    }

    // If the item was found, update its event
    if(ptListItemDes != &tListObject.tListEnd)
    {
        ptBaseDes = ptListItemDes->pvOwner;
        MODUS_ASSERT(NULL != ptBaseDes);

        ptBaseDes->wEvent |= wEvent;
    }
    else
    {
        // If the item was not found, return an error
        wRet = MODUS_ENODEV;
    }
    
    return wRet;
}

/**
 * Function: mbase_EventPend
 * ----------------------------
 * This function retrieves and clears the event of a modus_base_t structure. If the event is not zero, 
 * it clears the event and returns the original event.
 *
 * Parameters: 
 * ptBase: A pointer to the modus_base_t structure to retrieve the event from.
 *
 * Returns: 
 * The original event of the modus_base_t structure, or zero if the event was zero or ptBase is NULL.
 */
uint32_t mbase_EventPend(modus_base_t *ptBase)
{
    // Check for null pointer
    if (NULL == ptBase) {
        return 0;
    }

    uint32_t wEvent = ptBase->wEvent;
    if(0 != wEvent)
    {
        ptBase->wEvent = 0;
    }
    return wEvent;
}

/**
 * Function: mbase_MessagePost
 * ----------------------------
 * This function posts a message to a base object with a specific ID. It traverses a list of base objects, 
 * finds the one with the given ID, and updates its message and length. If no base object with the given ID 
 * is found, it returns an error code.
 *
 * Parameters: 
 * wId: The ID of the base object to post the message to.
 * ptMsgItem: A pointer to the message_item_t structure containing the message and its length.
 *
 * Returns: 
 * A status code indicating the result of the function. MODUS_SUCCESS if the function succeeds, 
 * MODUS_EINVAL if the ptMsgItem pointer is NULL, or MODUS_ENODEV if no base object with the given ID is found.
 */
int mbase_MessagePost(uint32_t wId, message_item_t *ptMsgItem)
{
    // Check for valid input
    if (wId == 0 || NULL == ptMsgItem) {
        return MODUS_EINVAL;
    }

    int wRet = MODUS_SUCCESS;
    mlist_item_t *ptListItemDes = tListObject.tListEnd.ptPrevious;
    modus_base_t *ptBaseDes;

    // Find the list item with the given ID
    for (; ptListItemDes != &tListObject.tListEnd; ptListItemDes = ptListItemDes->ptPrevious) {
        if(ptListItemDes->wItemValue == wId)
            break;
    }

    // If the item was found, update its message and length, and set a transition event
    if(ptListItemDes != &tListObject.tListEnd)
    {
        ptBaseDes = ptListItemDes->pvOwner;
        ptMsgItem->tListItem.pvOwner = ptMsgItem;
        MODUS_ASSERT(NULL != ptBaseDes);
        // if message length is not zero, insert message into list
        if(ptMsgItem->hwLength > 0)
        {
            mlist_Insert(&ptBaseDes->tListMessage, &ptMsgItem->tListItem);
        }
        
        ptBaseDes->wEvent |= Modus_Event_Transition;
    }
    else
    {
        // If the item was not found, return an error
        wRet = MODUS_ENODEV;
    }
    
    return wRet;
}

/**
 * Function: mbase_MessagePend
 * ----------------------------
 * This function retrieves and removes a message from a modus_base_t structure. It copies the message and its 
 * length to the message_t structure pointed to by ptMsg. If the message is successfully retrieved, the function 
 * returns the length of the message; otherwise, it returns an error code.
 *
 * Parameters: 
 * ptBase: A pointer to the modus_base_t structure to retrieve the message from.
 * ptMsg: A pointer to the message_t structure to copy the message and its length to.
 *
 * Returns: 
 * The length of the message, or an error code if the message was not successfully retrieved.
 */
int mbase_MessagePend(modus_base_t *ptBase, message_t *ptMsg)
{
    // Check for null pointer
    if (ptBase == NULL) {
        return MODUS_EINVAL;
    }

    int wRet = MODUS_SUCCESS;
    mlist_item_t *ptListItemDes = ptBase->tListMessage.tListEnd.ptPrevious;
    message_t *ptMsgDes;

    // Find the list item with the given ID
    if(ptListItemDes != &ptBase->tListMessage.tListEnd)
    {
        ptMsgDes = (message_t *)ptListItemDes->pvOwner;
        MODUS_ASSERT(NULL != ptMsgDes);

        // copy message and length
        ptMsg->hwLength = ptMsgDes->hwLength;
        memcpy(ptMsg->pchMessage, ptMsgDes->pchMessage, ptMsgDes->hwLength);

        // remove message from list
        mlist_Remove(ptListItemDes);

        wRet = ptMsg->hwLength;
    }
    else
    {
        // If the item was not found, return an error
        wRet = MODUS_ENODEV;
    }
    return wRet;
}

/**
 * Function: mbase_MessagePostToRing
 * ---------------------------------
 * This function posts a message to a ring buffer of a base object with a specific ID. It traverses a list 
 * of base objects, finds the one with the given ID, and updates its ring buffer with the message. If no base 
 * object with the given ID is found, it returns an error code.
 *
 * Parameters: 
 * wId: The ID of the base object to post the message to.
 * pchMsgBuffer: A pointer to the message buffer containing the message to post.
 * hwLength: The length of the message.
 *
 * Returns: 
 * A status code indicating the result of the function. MODUS_SUCCESS if the function succeeds, 
 * MODUS_EINVAL if any input parameter is invalid, or MODUS_ENODEV if no base object with the given ID is found.
 */
int mbase_MessagePostToRing(uint32_t wId, uint8_t *pchMsgBuffer, uint16_t hwLength)
{
    // Check for valid input
    if (wId == 0 || NULL == pchMsgBuffer || hwLength == 0) {
        return MODUS_EINVAL;
    }

    int wRet = MODUS_SUCCESS;
    mlist_item_t *ptListItemDes = tListObject.tListEnd.ptPrevious;
    modus_base_t *ptBaseDes;

    // Find the list item with the given ID
    for (; ptListItemDes != &tListObject.tListEnd; ptListItemDes = ptListItemDes->ptPrevious) {
        if(ptListItemDes->wItemValue == wId)
            break;
    }

    // If the item was found, update its message and length, and set a transition event
    if(ptListItemDes != &tListObject.tListEnd)
    {
        ptBaseDes = ptListItemDes->pvOwner;
        MODUS_ASSERT(NULL != ptBaseDes);
        object_ring_buffer_t *pRing = &ptBaseDes->tRingBuffer;
        if (pRing->buffer && pRing->hwBufferSize > 0) {
            for (uint16_t i = 0; i < hwLength; ++i) {
                pRing->buffer[pRing->hwWriteIndex] = pchMsgBuffer[i];
                pRing->hwWriteIndex = (pRing->hwWriteIndex + 1) % pRing->hwBufferSize;
                // If the write index catches up to the read index, increment the read index
                if (pRing->hwWriteIndex == pRing->hwReadIndex) {
                    pRing->hwReadIndex = (pRing->hwReadIndex + 1) % pRing->hwBufferSize;
                }
            }
        }
        ptBaseDes->wEvent |= Modus_Event_Transition;
    }
    else
    {
        // If the item was not found, return an error
        wRet = MODUS_ENODEV;
    }

    return wRet;
}

/**
 * Function: mbase_MessagePendFromRing
 * -----------------------------------
 * This function retrieves messages from a ring buffer of a base object. It reads messages from the ring 
 * buffer until it reaches the maximum size specified or until there are no more messages to read.
 *
 * Parameters: 
 * ptBase: A pointer to the modus_base_t structure containing the ring buffer.
 * pchMsgBuffer: A pointer to the buffer to copy the messages to.
 * hwMaxSize: The maximum size of the buffer.
 *
 * Returns: 
 * The number of bytes read from the ring buffer, or an error code if the input parameters are invalid.
 */
int mbase_MessagePendFromRing(modus_base_t *ptBase, uint8_t *pchMsgBuffer, uint16_t hwMaxSize)
{
    // Check for null pointer
    if (NULL == ptBase || NULL == pchMsgBuffer || hwMaxSize == 0) {
        return MODUS_EINVAL;
    }

    int wRet = 0;
    object_ring_buffer_t *ptRing = &ptBase->tRingBuffer;

    uint16_t hwLength = 0;
    while (ptRing->hwReadIndex != ptRing->hwWriteIndex && hwLength < hwMaxSize) {
        pchMsgBuffer[hwLength++] = ptRing->buffer[ptRing->hwReadIndex];
        ptRing->hwReadIndex = (ptRing->hwReadIndex + 1) % ptRing->hwBufferSize;
    }
    wRet = hwLength; // Return the number of bytes read, 0 if no data
    return wRet;
}

/**
 * Function: mbase_ShareMemInit
 * ----------------------------
 * This function initializes the shared memory for a base object. It checks if the base and shared memory 
 * pointers are not NULL, clears the shared memory buffer, and returns a status code indicating the result.
 *
 * Parameters: 
 * ptBase: A pointer to the modus_base_t structure to initialize.
 * ptShareMem: A pointer to the share_mem_t structure containing the shared memory configuration.
 *
 * Returns: 
 * MODUS_SUCCESS if the function runs successfully, MODUS_EINVAL if any of the pointers is NULL or if the 
 * shared memory buffer is NULL or has a size of zero.
 */
int mbase_ShareMemInit(modus_base_t *ptBase, share_mem_t *ptShareMem)
{
    // Check for null pointers
    if (NULL == ptBase || NULL == ptShareMem) {
        return MODUS_EINVAL;
    }

    // Initialize the shared memory structure
    ptBase->ptShareMem = ptShareMem;
    if(ptShareMem->pchBuffer != NULL && ptShareMem->hwSize > 0) {
        uint8_t *pchBuffer = (uint8_t *)ptShareMem->pchBuffer;
        // Clear the shared memory buffer
        memset(pchBuffer, 0, ptShareMem->hwSize);
    } else {
        // If the buffer is NULL or size is zero, return an error
        return MODUS_EINVAL;
    }

    return MODUS_SUCCESS;
}

/**
 * Function: mbase_ShareMemRead
 * ----------------------------
 * This function retrieves the shared memory pointer for a base object with a specific ID. It traverses 
 * a list of base objects, finds the one with the given ID, and returns its shared memory pointer. 
 * If no base object with the given ID is found, it returns NULL.
 *
 * Parameters: 
 * wId: The ID of the base object to retrieve the shared memory pointer from.
 *
 * Returns: 
 * A pointer to the shared memory structure of the base object, or NULL if no base object with the given ID is found.
 */
share_mem_t* mbase_ShareMemRead(uint32_t wId)
{
    // Check for valid input
    if (wId == 0) {
        return NULL;
    }

    mlist_item_t *ptListItemDes = tListObject.tListEnd.ptPrevious;
    modus_base_t *ptBaseDes;

    // Find the list item with the given ID
    for (; ptListItemDes != &tListObject.tListEnd; ptListItemDes = ptListItemDes->ptPrevious) {
        if(ptListItemDes->wItemValue == wId)
            break;
    }

    if(ptListItemDes != &tListObject.tListEnd)
    {
        ptBaseDes = ptListItemDes->pvOwner;
        MODUS_ASSERT(NULL != ptBaseDes);
        // Return the shared memory pointer
        return ptBaseDes->ptShareMem;
    }
    else
    {
        // If the item was not found, return NULL
        return NULL;
    }
}

/**
 * Function: mbase_DebugListBase
 * ----------------------------
 * This function prints the IDs of all objects in the global list object.
 *
 * Parameters: 
 * None
 *
 * Returns: 
 * None
 */
void mbase_DebugListBase(void)
{
    MLOG(D, "List all object:\n");

    /* Defensive: if the list hasn't been initialised, the end pointers may be NULL
       and walking them will crash. Detect an uninitialised list and print a
       friendly message instead. */
    if (tListObject.tListEnd.ptPrevious == NULL || tListObject.tListEnd.ptNext == NULL || tListObject.wNumberOfItems == 0) {
        MLOG(D, "    <empty>\n");
        return;
    }

    /* Traverse the list and print each item's ID */
    for (mlist_item_t *ptListItemDes = tListObject.tListEnd.ptPrevious; ptListItemDes != &tListObject.tListEnd; ptListItemDes = ptListItemDes->ptPrevious) {
        MLOGF(D, "    item id: 0x%x\n", (unsigned)ptListItemDes->wItemValue);
    }
}

/**
 * Function: mbase_GetBaseList
 * ----------------------------
 * This function returns a reference to the global list object.
 *
 * Parameters: 
 * None
 *
 * Returns: 
 * A pointer to the global list object.
 */
mlist_t* mbase_GetBaseList(void)
{
    // Return a reference to the global list object
    return &tListObject;
}
