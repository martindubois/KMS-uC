
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/Filter_FIR.c

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
#include "Filter_FIR.h"

// Functions
// //////////////////////////////////////////////////////////////////////////

void Filter_FIR_NewSample(Filter_FIR* aThis, int32_t aNewValue_FP)
{
    aThis->mCount++;
    aThis->mSum_FP += aNewValue_FP;

    if (aThis->mPeriod <= aThis->mCount)
    {
        aThis->mSum_FP /= aThis->mCount;
        aThis->mOutput_FP = aThis->mSum_FP;

        aThis->mCount  = 0;
        aThis->mSum_FP = 0;
    }
}

void Filter_FIR_Reset(Filter_FIR* aThis)
{
    aThis->mSum_FP = 0;
    aThis->mCount = 0;
    aThis->mOutput_FP = 0;
}
