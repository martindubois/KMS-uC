
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/Modbus_Slave.c

// References
// //////////////////////////////////////////////////////////////////////////
//
// MODBUS APPLICATION PROTOCOL SPECIFICATION V1.1b3
// https://modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf
//
// MODBUS over Serial Line Specification and Implementation Guide V1.02
// https://modbus.org/docs/Modbus_over_serial_line_V1_02.pdf

// Assumptions
// //////////////////////////////////////////////////////////////////////////

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////
//
// Project configuration
// - Also add Modbus_CRC.c to the project
// - Also add MC56F/QSCI.c to the project

// Code
// //////////////////////////////////////////////////////////////////////////

// ===== C ==================================================================
#include <stdint.h>
#include <stdlib.h>

// ===== Includes ===========================================================
#include "Modbus.h"
#include "Modbus_CRC.h"
#include "UART.h"

#include "Modbus_Slave.h"

// Configuration
// //////////////////////////////////////////////////////////////////////////

#define TIMEOUT_ms (500)

// Data types
// //////////////////////////////////////////////////////////////////////////

// --> INIT
//     |
//     +--> WAITING <====+------+
//          |            |      |
//          +--> READING |      |
//               | |     |      |
//               | +--> WRITING |
//               |      |       |
//               +------+==> ERROR
typedef enum
{
    STATE_ERROR  ,
    STATE_INIT   ,
    STATE_READING,
    STATE_WAITING,
    STATE_WRITING
}
State;

// Constants
// //////////////////////////////////////////////////////////////////////////

#define UNKNOWN_EXPECTED_COUNT (0xff)

// Variables
// //////////////////////////////////////////////////////////////////////////

static uint8_t             sBuffer[32];
static uint8_t             sCount;
static uint16_t            sData[16];
static uint8_t             sDevice;
static uint8_t             sExpectedCount;
static GPIO                sOutputEnable;
static uint8_t             sRangeQty;
static Modbus_Slave_Range* sRanges;
static State               sState;
static uint8_t             sUART;

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

// Return  Size of the answer excluding the CRC, in byte.
static uint8_t Execute_READ_REGISTERS          (uint16_t aAddr, uint16_t aCount);
static uint8_t Execute_WRITE_MULTIPLE_REGISTERS(uint16_t aAddr, uint16_t aCount);
static uint8_t Execute_WRITE_SINGLE_REGISTER   (uint16_t aAddr, uint16_t aValue);

// Return  NULL   No range find
//         Other  The pointer to the range
static Modbus_Slave_Range* FindRange(uint16_t aAddr, uint16_t aCount);

static void ParseRequest();

// Return  Size of the answer excluding the CRC, in byte.
static uint8_t Parse_READ_REGISTERS();
static uint8_t Parse_WRITE_MULTIPLE_REGISTERS();
static uint8_t Parse_WRITE_SINGLE_REGISTER();

static void Set_WAITING();
static void Set_WRITING(uint8_t aSize_byte);

static void Work_READING();
static void Work_WAITING();
static void Work_WRITING();

// Functions
// //////////////////////////////////////////////////////////////////////////

void Modbus_Slave_Init(uint8_t aUART, uint8_t aDevice, Modbus_Slave_Range* aRanges, uint8_t aRangeQty, GPIO aOutputEnable)
{
    // assert(0 < aDevice);
    // assert(NULL != aRanges);
    // assert(0 < aRangeQty);

    sDevice       = aDevice;
    sOutputEnable = aOutputEnable;
    sRanges       = aRanges;
    sRangeQty     = aRangeQty;
    sState        = STATE_INIT;
    sUART         = aUART;

    sOutputEnable.mOutput        = 1;
    sOutputEnable.mSlewRate_Slow = 1;

    UART_Init(sUART);

    GPIO_Output(sOutputEnable, 0);
}

uint8_t Modbus_Slave_Callback_Default(struct Modbus_Slave_Range_s* aRange, uint16_t aAddress, uint8_t aCount, uint16_t* aData)
{
    // assert(NULL != aRange);
    // assert(0 < aCount);
    // assert(NULL != aData);

    return MODBUS_NO_ERROR;
}

uint8_t Modbus_Slave_Callback_Error(struct Modbus_Slave_Range_s* aRange, uint16_t aAddress, uint8_t aCount, uint16_t* aData)
{
    // assert(NULL != aRange);
    // assert(0 < aCount);
    // assert(NULL != aData);

    return MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
}

void Modbus_Slave_Tick(uint16_t aPeriod_ms)
{
    // assert(0 < aPeriod_ms);

    switch (sState)
    {
    case STATE_ERROR: sState = STATE_INIT; break;

    case STATE_INIT: Set_WAITING(); break;

    case STATE_READING:
    case STATE_WAITING:
        UART_Tick(sUART, UART_READ, aPeriod_ms);
        break;

    case STATE_WRITING:
        UART_Tick(sUART, UART_WRITE, aPeriod_ms);
        break;

    // default: assert(false);
    }
}

void Modbus_Slave_Work()
{
    switch (sState)
    {
    case STATE_ERROR:
    case STATE_INIT: break;

    case STATE_READING: Work_READING(); break;
    case STATE_WAITING: Work_WAITING(); break;
    case STATE_WRITING: Work_WRITING(); break;

    // default: assert(false);
    }
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

uint8_t Execute_READ_REGISTERS(uint16_t aAddr, uint16_t aCount)
{
    // assert(0 < aCount);

    uint8_t lResult_byte = 1 + 1 + 1; // Device, Function, Exception

    Modbus_Slave_Range* lRange = FindRange(aAddr, aCount);
    if (NULL != lRange)
    {
        uint8_t lRet;

        unsigned int i;

        if (NULL == lRange->mData)
        {
            for (i = 0; i < aCount; i++)
            {
                sData[i] = 0;
            }
        }
        else
        {
            uint16_t lIndex = aAddr - lRange->mAddress;

            for (i = 0; i < aCount; i++)
            {
                sData[i] = lRange->mData[lIndex + i];
            }
        }

        lRet = lRange->mAfterRead(lRange, aAddr, aCount, sData);
        if (MODBUS_NO_ERROR == lRet)
        {
            lResult_byte = 1 + 1; // Device, Function

            sBuffer[lResult_byte] = sizeof(uint16_t) * aCount;
            lResult_byte++;

            for (i = 0; i < aCount; i++)
            {
                uint8_t lHigh = sData[i] >> 8;
                uint8_t lLow  = sData[i];

                sBuffer[lResult_byte    ] = lHigh;
                sBuffer[lResult_byte + 1] = lLow;

                lResult_byte += sizeof(uint16_t);
            }
        }
        else
        {
            sBuffer[MODBUS_BYTE_FUNCTION ] |= MODBUS_FUNCTION_ERROR;
            sBuffer[MODBUS_BYTE_EXCEPTION]  = lRet;
        }
    }
    else
    {
        sBuffer[MODBUS_BYTE_FUNCTION ] |= MODBUS_FUNCTION_ERROR;
        sBuffer[MODBUS_BYTE_EXCEPTION]  = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    return lResult_byte;
}

uint8_t Execute_WRITE_MULTIPLE_REGISTERS(uint16_t aAddr, uint16_t aCount)
{
    // assert(0 < aCount);

    uint8_t lResult_byte = 1 + 1 + 1; // Device, Function, Exception

    Modbus_Slave_Range* lRange = FindRange(aAddr, aCount);
    if (NULL != lRange)
    {
        uint8_t  lByte  = 7;
        uint16_t lIndex = aAddr - lRange->mAddress;
        int      lRet   = 0;

        uint8_t i;

        for (i = 0; i < aCount; i++)
        {
            sData[i] = (sBuffer[lByte] << 8) | sBuffer[lByte + 1];

            lByte += sizeof(uint16_t);
        }

        lRet = lRange->mBeforeWrite(lRange, aAddr, aCount, sData);
        if (MODBUS_NO_ERROR == lRet)
        {
            if (NULL != lRange->mData)
            {
                for (i = 0; i < aCount; i++)
                {
                    lRange->mData[lIndex + i] = sData[i];
                }
            }

            lRet = lRange->mAfterWrite(lRange, aAddr, aCount, sData);
            if (0 == lRet)
            {
                lResult_byte = 1 + 1 + sizeof(uint16_t) + sizeof(uint16_t); // Device, Function, Address, Count
            }
        }

        if (MODBUS_NO_ERROR != lRet)
        {
            sBuffer[MODBUS_BYTE_FUNCTION ] |= MODBUS_FUNCTION_ERROR;
            sBuffer[MODBUS_BYTE_EXCEPTION]  = lRet;
        }
    }
    else
    {
        sBuffer[MODBUS_BYTE_FUNCTION ] |= MODBUS_FUNCTION_ERROR;
        sBuffer[MODBUS_BYTE_EXCEPTION]  = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    return lResult_byte;
}

uint8_t Execute_WRITE_SINGLE_REGISTER(uint16_t aAddr, uint16_t aValue)
{
    uint8_t lResult_byte = 1 + 1 + 1; // Device, Function, Exception

    Modbus_Slave_Range* lRange = FindRange(aAddr, 1);
    if (NULL != lRange)
    {
        uint16_t lIndex = aAddr - lRange->mAddress;
        uint8_t  lRet;

        sData[0] = aValue;

        lRet = lRange->mBeforeWrite(lRange, aAddr, 1, sData);
        if (MODBUS_NO_ERROR == lRet)
        {
            if (NULL != lRange->mData)
            {
                lRange->mData[lIndex] = sData[0];
            }

            lRet = lRange->mAfterWrite(lRange, aAddr, 1, sData);
            if (0 == lRet)
            {
                uint8_t lHigh = sData[0] >> 8;
                uint8_t lLow  = sData[0];

                sBuffer[4] = lHigh;
                sBuffer[5] = lHigh;

                lResult_byte = 1 + 1 + sizeof(uint16_t) + sizeof(uint16_t); // Device, Function, Address, Value
            }
        }

        if (MODBUS_NO_ERROR != lRet)
        {
            sBuffer[MODBUS_BYTE_FUNCTION ] |= MODBUS_FUNCTION_ERROR;
            sBuffer[MODBUS_BYTE_EXCEPTION]  = lRet;
        }
    }
    else
    {
        sBuffer[MODBUS_BYTE_FUNCTION ] |= MODBUS_FUNCTION_ERROR;
        sBuffer[MODBUS_BYTE_EXCEPTION]  = MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    }

    return lResult_byte;
}

Modbus_Slave_Range* FindRange(uint16_t aAddr, uint16_t aCount)
{
    // assert(0 < aCount);

    uint16_t lEnd = aAddr + aCount;

    uint8_t i;

    for (i = 0; i < sRangeQty; i++)
    {
        uint16_t lRA = sRanges[i].mAddress;

        if ((lRA <= aAddr) && ((lRA + sRanges[i].mCount) >= lEnd))
        {
            return sRanges + i;
        }
    }

    return NULL;
}

void ParseRequest()
{
    if (UNKNOWN_EXPECTED_COUNT == sExpectedCount)
    {
        if (MODBUS_BYTE_FUNCTION < sCount)
        {
            switch (sBuffer[MODBUS_BYTE_FUNCTION])
            {
            case MODBUS_FUNCTION_READ_HOLDING_REGISTERS:
            case MODBUS_FUNCTION_READ_INPUT_REGISTERS  :
                sExpectedCount = 1 + 1 + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t); // Device, Function, Address, Count, CRC
                break;

            case MODBUS_FUNCTION_WRITE_SINGLE_REGISTER:
                sExpectedCount = 1 + 1 + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t); // Device, Function, Address, Value, CRC
                break;
            }
        }
        if (6 < sCount)
        {
            switch (sBuffer[MODBUS_BYTE_FUNCTION])
            {
            case MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS:
                sExpectedCount  = 1 + 1 + sizeof(uint16_t) + sizeof(uint16_t) + 1; // Device, Function, Address, Count, Size_byte
                sExpectedCount += sBuffer[6];
                sExpectedCount += 2; // CRC
                break;

            default:
                UART_Abort(sUART, UART_READ);
                sState = STATE_ERROR;
            }
        }
    }

    if (sExpectedCount <= sCount)
    {
        UART_Abort(sUART, UART_READ);

        if (Modbus_CRC_Verify_Buffer(sBuffer, sCount) && (sDevice == sBuffer[MODBUS_BYTE_DEVICE]))
        {
            uint8_t lSize_byte = 1 + 1 + 1; // Device, Function, Exception

            switch (sBuffer[MODBUS_BYTE_FUNCTION])
            {
            case MODBUS_FUNCTION_READ_HOLDING_REGISTERS:
            case MODBUS_FUNCTION_READ_INPUT_REGISTERS  : lSize_byte = Parse_READ_REGISTERS(); break;

            case MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS: lSize_byte = Parse_WRITE_MULTIPLE_REGISTERS(); break;
            case MODBUS_FUNCTION_WRITE_SINGLE_REGISTER   : lSize_byte = Parse_WRITE_SINGLE_REGISTER   (); break;

            default:
                sBuffer[MODBUS_BYTE_FUNCTION ] |= MODBUS_FUNCTION_ERROR;
                sBuffer[MODBUS_BYTE_EXCEPTION]  = MODBUS_EXCEPTION_ILLEGAL_FUNCTION;
            }

            Modbus_CRC_Compute_Buffer(sBuffer, lSize_byte);

            lSize_byte += sizeof(uint16_t); // CRC

            Set_WRITING(lSize_byte);
        }
        else
        {
            Set_WAITING();
        }
    }
}

// Device 0x03 AddrH AddrL CountH CountL
// Device 0x04 AddrH AddrL CountH CountL
uint8_t Parse_READ_REGISTERS()
{
    uint16_t lAddr  = (sBuffer[2] << 8) | sBuffer[3];
    uint16_t lCount = (sBuffer[4] << 8) | sBuffer[5];

    return Execute_READ_REGISTERS(lAddr, lCount);
}

// Device 0x10 AddrH AddrL CountH CountL ByteCount ...
uint8_t Parse_WRITE_MULTIPLE_REGISTERS()
{
    uint16_t lAddr  = (sBuffer[2] << 8) | sBuffer[3];
    uint16_t lCount = (sBuffer[4] << 8) | sBuffer[5];

    return Execute_WRITE_MULTIPLE_REGISTERS(lAddr, lCount);
}

// Device 0x06 AddrH AddrL ValueH ValueL
uint8_t Parse_WRITE_SINGLE_REGISTER()
{
    uint16_t lAddr  = (sBuffer[2] << 8) | sBuffer[3];
    uint16_t lValue = (sBuffer[4] << 8) | sBuffer[5];

    return Execute_WRITE_SINGLE_REGISTER(lAddr, lValue);
}

void Set_WAITING()
{
    UART_Read(sUART, sBuffer, sizeof(sBuffer));

    sState = STATE_WAITING;
}

void Set_WRITING(uint8_t aSize_byte)
{
    // assert(4 <= aSize_byte);

    GPIO_Output(sOutputEnable, 1);

    UART_Write(sUART, sBuffer, aSize_byte);

    sState = STATE_WRITING;
}

void Work_READING()
{
    uint8_t lCount;

    switch (UART_Status(sUART, UART_READ, &lCount))
    {
    case UART_ERROR: sState = STATE_ERROR; break;

    case UART_PENDING:
        if (sCount < lCount)
        {
            sCount = lCount;

            ParseRequest();
        }
        break;

    // default: assert(false);
    }
}

void Work_WAITING()
{
    uint8_t lCount;

    switch (UART_Status(sUART, UART_READ, &lCount))
    {
    case UART_ERROR: sState = STATE_ERROR; break;

    case UART_PENDING:
        if (0 < lCount)
        {
            UART_SetTimeout(sUART, UART_READ, TIMEOUT_ms);

            sCount         = lCount;
            sExpectedCount = UNKNOWN_EXPECTED_COUNT;
            sState         = STATE_READING;

            ParseRequest();
        }
        break;

    // default: assert(false);
    }
}

void Work_WRITING()
{
    uint8_t lCount;

    switch (UART_Status(sUART, UART_WRITE, &lCount))
    {
    case UART_ERROR:
        GPIO_Output(sOutputEnable, 0);
        sState = STATE_ERROR;
        break;

    case UART_PENDING: break;

    case UART_SUCCESS:
        GPIO_Output(sOutputEnable, 0);
        Set_WAITING();
        break;

    // default: assert(false);
    }
}
