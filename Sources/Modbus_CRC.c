
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/Modbus_CRC.c

// References
// //////////////////////////////////////////////////////////////////////////

// Assumptions
// //////////////////////////////////////////////////////////////////////////

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////

// Code
// //////////////////////////////////////////////////////////////////////////

// ===== C ==================================================================
#include <stdint.h>

// ===== Includes ===========================================================
#include "Modbus_CRC.h"

// Functions
// //////////////////////////////////////////////////////////////////////////

void Modbus_CRC_Compute_Buffer(uint8_t* aInOut, uint8_t aInSize_byte)
{
    // assert(0 < aInSize_byte);

    Modbus_CRC lCRC;

    uint8_t i;

    Modbus_CRC_Init(&lCRC);

    for (i = 0; i < aInSize_byte; i++)
    {
        Modbus_CRC_Compute_Byte(&lCRC, aInOut[i]);
    }

    Modbus_CRC_Get(&lCRC, aInOut + aInSize_byte);
}

uint8_t Modbus_CRC_Verify_Buffer(const uint8_t* aIn, uint8_t aInSize_byte)
{
    // assert(2 < aInSize_byte);

    Modbus_CRC lCRC;
    uint8_t    lInSize_byte = aInSize_byte - sizeof(uint16_t);

    uint8_t i;

    Modbus_CRC_Init(&lCRC);

    for (i = 0; i < lInSize_byte; i++)
    {
        Modbus_CRC_Compute_Byte(&lCRC, aIn[i]);
    }

    return Modbus_CRC_Verify(&lCRC, aIn + lInSize_byte);
}

void Modbus_CRC_Init(Modbus_CRC* aThis) { aThis->mValue = 0xffff; }

void Modbus_CRC_Get(const Modbus_CRC* aThis, uint8_t* aOut)
{
    // assert(NULL != aOut);

    uint8_t lHigh;
    uint8_t lLow;

    lHigh = (uint8_t)(aThis->mValue >> 8);
    lLow  = (uint8_t) aThis->mValue;

    aOut[0] = lLow;
    aOut[1] = lHigh;
}

void Modbus_CRC_Compute_Byte(Modbus_CRC* aThis, uint8_t aByte)
{
    uint16_t i;

    aThis->mValue ^= aByte;

    for (i = 8; i > 0; i--)
	{
        if (1 == (aThis->mValue & 0x0001))
		{
            aThis->mValue >>= 1;
            aThis->mValue ^= 0xA001;
        }
		else
		{
            aThis->mValue >>= 1;
		}
    }
}

uint8_t Modbus_CRC_Verify(const Modbus_CRC* aThis, const uint8_t* aIn)
{
    // assert(NULL != aIn);

    uint8_t lHigh;
    uint8_t lLow;

    lHigh = (uint8_t)(aThis->mValue >> 8);
    lLow  = (uint8_t) aThis->mValue;

    return (aIn[0] == lLow) && (aIn[1] == lHigh);
}
