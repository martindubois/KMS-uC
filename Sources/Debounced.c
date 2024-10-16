
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

// Code
// //////////////////////////////////////////////////////////////////////////

// ===== C ==================================================================
#include <stdint.h>

// ===== Includes ===========================================================
#include "Debounced.h"

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static uint8_t GetRawValue(Debounced* aThis);

// Functions
// //////////////////////////////////////////////////////////////////////////

void Debounced_Init(Debounced* aThis, volatile uint16_t* aRegister, uint16_t aMask)
{
    aThis->mCounter  = 0;
    aThis->mMask     = aMask;
    aThis->mRegister = aRegister;

    aThis->mValue = GetRawValue(aThis);
}

uint8_t Debounced_GetValue(Debounced* aThis)
{
    uint8_t lRawValue = GetRawValue(aThis);
    if (aThis->mValue == lRawValue)
    {
        aThis->mCounter = 0;
    }
    else
    {
        aThis->mCounter++;

        if (3 <= aThis->mCounter)
        {
            aThis->mCounter = 0;
            aThis->mValue   = lRawValue;
        }
    }

    return aThis->mValue;
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

uint8_t GetRawValue(Debounced* aThis)
{
    return 0 != (*aThis->mRegister & aThis->mMask);
}
