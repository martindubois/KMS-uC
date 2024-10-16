
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/EEPROM.c

// References
// //////////////////////////////////////////////////////////////////////////

// Assumptions
// //////////////////////////////////////////////////////////////////////////

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////
//
// Project configuration
// - Also add MC56F/I2C.c to the project

// Code
// //////////////////////////////////////////////////////////////////////////
//
// - Call I2C_Init

// ===== C ==================================================================
#include <stdint.h>
#include <string.h>

// ===== Includes ===========================================================
#include "I2C.h"

#include "EEPROM.h"

// Constants
// //////////////////////////////////////////////////////////////////////////

#define ERASE_BYTE (0xff)

#define READ_MAX_byte  (16)
#define WRITE_MAX_byte (16)

#define WRITE_PROTECT_ON  (1)
#define WRITE_PROTECT_OFF (0)

#define TIMEOUT_ms (500)

// --> IDLE <==+=============+------+
//      |      |             |      |
//      |      | +------+====|====> ERROR <===+==+=========+==+=====+==+--------+
//      |      | |      |    |      ^         |  |         |  |     |  |        |
//      +--> ERASE <--+ |    |      |         |  |         |  |     |  |        |
//      |    |        | |    |      |         |  |         |  |     |  |        |
//      |    +--> ERASE_WAIT |      |         |  |         |  |     |  |        |
//      |         |          |      |         |  |         |  |     |  |        |
//      |         +--> COMPLETED <==|======+==|==|======+==|==|==+--|--|------+ |
//      |                           |      |  |  |      |  |  |  |  |  |      | |
//      +---------> ERASE_VERIFY_ADDR <--+ |  |  |      |  |  |  |  |  |      | |
//      |                |               | |  |  |      |  |  |  |  |  |      | |
//      |                +--> ERASE_VERIFY_DATA  |      |  |  |  |  |  |      | |
//      |                                        |      |  |  |  |  |  |      | |
//      +------------------------------> READ_ADDR <--+ |  |  |  |  |  |      | |
//      |                                     |       | |  |  |  |  |  |      | |
//      |                                     +--> READ_DATA  |  |  |  |      | |
//      |                                                     |  |  |  |      | |
//      +-----------------------------------------> VERIFY_ADDR  |  |  |      | |
//      |                                            |           |  |  |      | |
//      |                                            +--> VERIFI_DATA  |      | |
//      |                                                              |      | |
//      +--------------------------------------------------------> WRITE <--+ | |
//                                                                 |        | | |
//                                                                 +--> WRITE_WAIT
#define STATE_IDLE (0)

#define STATE_COMPLETED         (1)
#define STATE_ERASE             (2)
#define STATE_ERASE_VERIFY_ADDR (3)
#define STATE_ERASE_VERIFY_DATA (4)
#define STATE_ERASE_WAIT        (5)
#define STATE_ERROR             (6)
#define STATE_READ_ADDR         (7)
#define STATE_READ_DATA         (8)
#define STATE_VERIFY_ADDR       (9)
#define STATE_VERIFY_DATA       (10)
#define STATE_WRITE             (11)
#define STATE_WRITE_WAIT        (12)

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static void Buffer_Clear(EEPROM* aThis);

static void Start_AddressState(EEPROM* aThis, unsigned int aNextState);
static void Start_ReadState   (EEPROM* aThis, unsigned int aNextState);
static void Start_WriteState  (EEPROM* aThis, unsigned int aNextState);
static void Start_WaitState   (EEPROM* aThis, unsigned int aNextState);

static void Work_WriteState(EEPROM* aThis, unsigned int aNextState);

static void Work_ERASE_VERIFY_ADDR(EEPROM* aThis);
static void Work_ERASE_VERIFY_DATA(EEPROM* aThis);
static void Work_ERASE_WAIT       (EEPROM* aThis);
static void Work_READ_ADDR        (EEPROM* aThis);
static void Work_READ_DATA        (EEPROM* aThis);
static void Work_VERIFY_ADDR      (EEPROM* aThis);
static void Work_VERIFY_DATA      (EEPROM* aThis);
static void Work_WRITE_WAIT       (EEPROM* aThis);

// Functions
// //////////////////////////////////////////////////////////////////////////

void EEPROM_Init(EEPROM* aThis, uint8_t aBusIndex, uint8_t aDeviceAddress, GPIO aWriteProtect)
{
    I2C_Device_Init(&aThis->mDevice, aBusIndex, aDeviceAddress);

    aThis->mWriteProtect = aWriteProtect;

    GPIO_Init(aThis->mWriteProtect);
}

void EEPROM_Erase(EEPROM* aThis, uint16_t aAddress, uint16_t aSize_byte)
{
    // assert(0 < aSize_byte);

    aThis->mAddress       = aAddress;
    aThis->mDataPtr       = aThis->mBuffer;
    aThis->mDataSize_byte = aSize_byte;
    
    Buffer_Clear(aThis);

    GPIO_Output(aThis->mWriteProtect, WRITE_PROTECT_OFF);

    Start_WriteState(aThis, STATE_ERASE);
}

void EEPROM_Erase_Verify(EEPROM* aThis, uint16_t aAddress, uint16_t aSize_byte)
{
    // assert(0 < aSize_byte);

    aThis->mAddress       = aAddress;
    aThis->mDataSize_byte = aSize_byte;

    Start_AddressState(aThis, STATE_ERASE_VERIFY_ADDR);
}

uint8_t EEPROM_Idle(EEPROM* aThis) { return STATE_IDLE == aThis->mState; }

void EEPROM_Read(EEPROM* aThis, uint16_t aAddress, void* aOut, uint16_t aOutSize_byte)
{
    // assert(NULL != aOut);
    // assert(0 < aOutSize_byte);

    aThis->mAddress       = aAddress;
    aThis->mDataPtr       = aOut;
    aThis->mDataSize_byte = aOutSize_byte;

    Start_AddressState(aThis, STATE_READ_ADDR);
}

uint8_t EEPROM_Status(EEPROM* aThis)
{
    uint8_t lResult = EEPROM_ERROR;

    switch (aThis->mState)
    {
    case STATE_COMPLETED:
        lResult = EEPROM_SUCCESS;
        aThis->mState = STATE_IDLE;
        break;

    case STATE_ERASE:
    case STATE_ERASE_VERIFY_ADDR:
    case STATE_ERASE_VERIFY_DATA:
    case STATE_ERASE_WAIT:
    case STATE_READ_ADDR:
    case STATE_READ_DATA:
    case STATE_VERIFY_ADDR:
    case STATE_VERIFY_DATA:
    case STATE_WRITE:
    case STATE_WRITE_WAIT:
        lResult = EEPROM_PENDING;
        break;

    case STATE_ERROR:
        break;

    // default: assert(false);
    }

    return lResult;
}

void EEPROM_Verify(EEPROM* aThis, uint16_t aAddress, const void* aIn, uint16_t aInSize_byte)
{
    // assert(NULL != aIn);
    // assert(0 < aInSize_byte);

    aThis->mAddress       = aAddress;
    aThis->mDataPtr       = (void*)aIn;
    aThis->mDataSize_byte = aInSize_byte;

    Start_AddressState(aThis, STATE_VERIFY_ADDR);
}

void EEPROM_Write(EEPROM* aThis, uint16_t aAddress, const void* aIn, uint16_t aInSize_byte)
{
    // assert(NULL != aIn);
    // assert(0 < aInSize_byte);

    aThis->mAddress       = aAddress;
    aThis->mDataPtr       = (void*)aIn;
    aThis->mDataSize_byte = aInSize_byte;

    Start_WriteState(aThis, STATE_WRITE);
}

void EEPROM_Tick(EEPROM* aThis, uint16_t aPeriod_ms)
{
    switch (aThis->mState)
    {
    case STATE_COMPLETED:
    case STATE_ERROR:
    case STATE_IDLE:
        // assert(0 == aThis->mTimeout_ms);
        break;

    case STATE_ERASE:
    case STATE_ERASE_VERIFY_ADDR:
    case STATE_ERASE_VERIFY_DATA:
    case STATE_READ_ADDR:
    case STATE_READ_DATA:
    case STATE_VERIFY_ADDR:
    case STATE_VERIFY_DATA:
    case STATE_WRITE:
        // assert(0 == aThis->mTimeout_ms);

        I2C_Device_Tick(aThis->mDevice, aPeriod_ms);
        break;

    case STATE_ERASE_WAIT:
    case STATE_WRITE_WAIT:
        // assert(0 < aThis->mTimeout_ms);

        I2C_Device_Tick(aThis->mDevice, aPeriod_ms);
        if (aThis->mTimeout_ms <= aPeriod_ms)
        {
            aThis->mTimeout_ms = 0;
            aThis->mState = STATE_ERROR;
        }
        else
        {
            aThis->mTimeout_ms -= aPeriod_ms;
        }
        break;

    // default: assert(false);
    }
}

void EEPROM_Work(EEPROM* aThis)
{
    switch (aThis->mState)
    {
    case STATE_COMPLETED:
    case STATE_ERROR:
    case STATE_IDLE:
        break;

    case STATE_ERASE            : Work_WriteState       (aThis, STATE_ERASE_WAIT); break;
    case STATE_ERASE_VERIFY_ADDR: Work_ERASE_VERIFY_ADDR(aThis); break;
    case STATE_ERASE_VERIFY_DATA: Work_ERASE_VERIFY_DATA(aThis); break;
    case STATE_ERASE_WAIT       : Work_ERASE_WAIT       (aThis); break;
    case STATE_READ_ADDR        : Work_READ_ADDR        (aThis); break;
    case STATE_READ_DATA        : Work_READ_DATA        (aThis); break;
    case STATE_VERIFY_ADDR      : Work_VERIFY_ADDR      (aThis); break;
    case STATE_VERIFY_DATA      : Work_VERIFY_DATA      (aThis); break;
    case STATE_WRITE            : Work_WriteState       (aThis, STATE_WRITE_WAIT); break;
    case STATE_WRITE_WAIT       : Work_WRITE_WAIT       (aThis); break;

    // default: assert(false);
    }
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

void Buffer_Clear(EEPROM* aThis)
{
    memset(&aThis->mBuffer, ERASE_BYTE, sizeof(aThis->mBuffer));
}

void Start_AddressState(EEPROM* aThis, unsigned int aNextState)
{
    // assert((STATE_ERASE_VERIFY_ADDR == aNextState) || (STATE_READ_ADDR == aNextState) || (STATE_VERIFY_ADDR == aNextState));

    // assert((STATE_IDLE == aThis->mState) || (STATE_ERASE_VERIFY_DATA == mState) || (STATE_READ_DATA == mState) || STATE_VERIFY_DATA == mState));

    I2C_Device_Write(aThis->mDevice, aThis->mAddress, NULL, 0);
    aThis->mState = aNextState;
}

void Start_ReadState(EEPROM* aThis, unsigned int aNextState)
{
    // assert((STATE_ERASE_VERIFY_DATA == aNextState) || (STATE_READ_DATA == aNextState) || (STATE_VERIFY_DATA == aNextState));

    // assert(0 < aThis->mDataSize_byte);
    // assert((STATE_ERASE_VERYFY_ADDR == aThis->mState) || (STATE_READ_ADDR == aThis->mData) || (STATE_VERIFY_ADDR == aThis->mState));

    aThis->mReadSize_byte = aThis->mDataSize_byte;
    if (READ_MAX_byte < aThis->mReadSize_byte)
    {
        aThis->mReadSize_byte = READ_MAX_byte;
    }

    I2C_Device_Read(aThis->mDevice, aThis->mDataPtr, aThis->mReadSize_byte);
    aThis->mState = aNextState;
}

void Start_WriteState(EEPROM* aThis, unsigned int aNextState)
{
    // assert((STATE_ERASE == aNextState) || STATE_WRITE == aNextState);

    // assert(0 < aThis->mDataSize_byte);
    // assert((STATE_IDLE == aThis->mState) || (STEATE_ERASE_WAIT == aThis->mState) || (STATE_WRITE_WAIT == aThis->mState))

    unsigned int lSize_byte = aThis->mDataSize_byte;
    if (WRITE_MAX_byte < lSize_byte)
    {
        lSize_byte = WRITE_MAX_byte;
    }

    I2C_Device_Write(aThis->mDevice, aThis->mAddress, aThis->mDataPtr, lSize_byte);

    aThis->mAddress       += lSize_byte;
    aThis->mDataPtr       += lSize_byte;
    aThis->mDataSize_byte -= lSize_byte;
    aThis->mState          = aNextState;
}

void Start_WaitState(EEPROM* aThis, unsigned int aNextState)
{
    // assert((STATE_ERASE_WAIT == aNextState) || (STATE_WRITE_WAIT == aNextState));

    // assert((STATE_ERASE == aThis->mState) || (STATE_ERASE_WAIT == aThis->mState) || (STATE_WRITE == aThis->mState) || (STATE_WRITE_WAIT == aThis->mState));
    // assert(0 == aThis->mTimeout_ms);

    I2C_Device_Write(aThis->mDevice, 0, NULL, 0);
    aThis->mState = aNextState;
    aThis->mTimeout_ms = TIMEOUT_ms;
}

void Work_WriteState(EEPROM* aThis, unsigned int aNextState)
{
    // assert((STATE_ERASE_WAIT == aNextState) || (STATE_WRITE_WAIT == aNextState));

    // assert((STATE_ERASE == aThis->mState) || (STATE_WRITE == aThis->mState));

    switch (I2C_Device_Status(aThis->mDevice))
    {
    case I2C_ERROR:
        GPIO_Output(aThis->mWriteProtect, WRITE_PROTECT_ON);
        aThis->mState = STATE_ERROR;
        break;

    case I2C_PENDING: break;

    case I2C_SUCCESS: Start_WaitState(aThis, aNextState); break;

    // default: assert(false);
    }
}

void Work_ERASE_VERIFY_ADDR(EEPROM* aThis)
{
    // assert(STATE_ERASE_VERIFY_ADDR == aThis->mState);

    switch (I2C_Device_Status(aThis->mDevice))
    {
    case I2C_ERROR: aThis->mState = STATE_ERROR; break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        aThis->mDataPtr = aThis->mBuffer;
        Start_ReadState(aThis, STATE_ERASE_VERIFY_DATA);
        break;

    // default: assert(false);
    }
}

void Work_ERASE_VERIFY_DATA(EEPROM* aThis)
{
    // assert(0 < aThis->mDataSize_byte);
    // assert(0 < aThis->mReadSize_byte);
    // assert(aThis->mDataSize_byte >= aThis->mReadSize_byte);
    // assert(STATE_ERASE_VERIFY_DATA == aThis->mState);

    unsigned int i;

    switch (I2C_Device_Status(aThis->mDevice))
    {
    case I2C_ERROR: aThis->mState = STATE_ERROR; break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        for (i = 0; i < aThis->mReadSize_byte; i++)
        {
            if (ERASE_BYTE != aThis->mBuffer[i])
            {
                aThis->mState = STATE_ERROR;
                return;
            }
        }

        aThis->mDataSize_byte -= aThis->mReadSize_byte;
        if (0 < aThis->mDataSize_byte)
        {
            aThis->mAddress += aThis->mReadSize_byte;

            Start_AddressState(aThis, STATE_ERASE_VERIFY_ADDR);
        }
        else
        {
            aThis->mState = STATE_COMPLETED;
        }
        break;

    // default: assert(false);
    }
}

void Work_ERASE_WAIT(EEPROM* aThis)
{
    // assert(STATE_ERASE_WAIT == aThis->mState);
    // assert(0 < aThis->mTimeout_ms);
    // assert(aThis->mWriteProtect.mOutput);

    switch (I2C_Device_Status(aThis->mDevice))
    {
    case I2C_ERROR: Start_WaitState(aThis, STATE_ERASE_WAIT); break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        if (0 < aThis->mDataSize_byte)
        {
            aThis->mDataPtr = aThis->mBuffer;

            Start_WriteState(aThis, STATE_ERASE);
        }
        else
        {
            GPIO_Output(aThis->mWriteProtect, WRITE_PROTECT_ON);
            aThis->mState = STATE_COMPLETED;
            aThis->mTimeout_ms = 0;
        }
        break;

    // default: assert(false);
    }
}

void Work_READ_ADDR(EEPROM* aThis)
{
    // assert(STATE_READ_ADDR == aThis->mState);

    switch (I2C_Device_Status(aThis->mDevice))
    {
    case I2C_ERROR: aThis->mState = STATE_ERROR; break;

    case I2C_PENDING: break;

    case I2C_SUCCESS: Start_ReadState(aThis, STATE_READ_DATA); break;

    // default: assert(false);
    }
}

void Work_READ_DATA(EEPROM* aThis)
{
    // assert(0 < aThis->mDataSize_byte);
    // assert(0 < aThis->mReadSize_byte);
    // assert(aThis->mDataSize_byte >= aThis->mReadSize_byte);
    // assert(STATE_READ_DATA == aThis->mState);

    switch (I2C_Device_Status(aThis->mDevice))
    {
    case I2C_ERROR: aThis->mState = STATE_ERROR; break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        aThis->mDataPtr       += aThis->mReadSize_byte;
        aThis->mDataSize_byte -= aThis->mReadSize_byte;
        if (0 < aThis->mDataSize_byte)
        {
            aThis->mAddress += aThis->mReadSize_byte;

            Start_AddressState(aThis, STATE_READ_ADDR);
        }
        else
        {
            aThis->mState = STATE_COMPLETED;
        }
        break;

    // default: assert(false);
    }
}

void Work_VERIFY_ADDR(EEPROM* aThis)
{
    // assert(STATE_VERIFY_ADDR == aThis->mState);

    switch (I2C_Device_Status(aThis->mDevice))
    {
    case I2C_ERROR: aThis->mState = STATE_ERROR; break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        aThis->mDataPtr = aThis->mBuffer;

        Start_ReadState(aThis, STATE_VERIFY_DATA);
        break;

    // default: assert(false);
    }
}

void Work_VERIFY_DATA(EEPROM* aThis)
{
    // assert(0 < aThis->mDataSize_byte);
    // assert(0 < aThis->mReadSize_byte);
    // assert(aThis->mDataSize_byte >= aThis->mReadSize_byte);
    // assert(STATE_VERIFY_DATA == aThis->mState);

    unsigned int i;

    switch (I2C_Device_Status(aThis->mDevice))
    {
    case I2C_ERROR: aThis->mState = STATE_ERROR; break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        for (i = 0; i < aThis->mReadSize_byte; i++)
        {
            if (aThis->mDataPtr[i] != aThis->mBuffer[i])
            {
                aThis->mState = STATE_ERROR;
                return;
            }
        }

        aThis->mDataPtr       += aThis->mReadSize_byte;
        aThis->mDataSize_byte -= aThis->mReadSize_byte;
        if (0 < aThis->mDataSize_byte)
        {
            aThis->mAddress += aThis->mReadSize_byte;

            Start_AddressState(aThis, STATE_VERIFY_ADDR);
        }
        else
        {
            aThis->mState = STATE_COMPLETED;
        }
        break;

    // default: assert(false);
    }
}

void Work_WRITE_WAIT(EEPROM* aThis)
{
    // assert(STATE_WRITE_WAIT == aThis->mState);
    // assert(0 < aThis->mTimeout_ms);
    // assert(aThis->mWriteProtect.mOutput);

    switch (I2C_Device_Status(aThis->mDevice))
    {
    case I2C_ERROR: Start_WaitState(aThis, STATE_WRITE_WAIT); break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        if (0 < aThis->mDataSize_byte)
        {
            Start_WriteState(aThis, STATE_WRITE);
        }
        else
        {
            GPIO_Output(aThis->mWriteProtect, WRITE_PROTECT_ON);
            aThis->mState = STATE_COMPLETED;
            aThis->mTimeout_ms = 0;
        }
        break;

    // default: assert(false);
    }
}
