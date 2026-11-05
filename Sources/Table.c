
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025-2026 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/Table.c

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
#include "Table.h"

// Functions
// //////////////////////////////////////////////////////////////////////////

int16_t Table_GetValue(const Table* aThis, int32_t aInput_FP)
{
    // assert(0 < aThis->mLength);
    // assert(0 < aThis->mStep_FP);
    // assert(NULL != aThis->mValues);

    int32_t lIndex = aInput_FP / aThis->mStep_FP;
    int16_t lResult;

    if (0 >= lIndex)
    {
        lResult = aThis->mValues[0];
    }
    else if (aThis->mLength <= lIndex)
    {
        lResult = aThis->mValues[aThis->mLength - 1];
    }
    else
    {
        lResult = aThis->mValues[lIndex];
    }

    return lResult;
}
