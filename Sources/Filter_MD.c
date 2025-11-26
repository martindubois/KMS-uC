
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/Filter_MD.c

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
#include "Filter_MD.h"

// Functions
// //////////////////////////////////////////////////////////////////////////

void Filter_MD_Init(Filter_MD* aThis, const Filter_MD_Table* aTable, Filter_MD_InputFunction aActual)
{
    aThis->mActual = aActual;
    aThis->mCounter_ms = 0;
    aThis->mInput_FP = 0;
    aThis->mOutput_FP = 0;
    aThis->mTable  = aTable;
}

// aThis
// aInput_FP  (fixed point 24.8)
inline void Filter_MD_SetInput(Filter_MD* aThis, int32_t aInput_FP)
{
    if (aThis->mInput_FP != aInput_FP)
    {
        aThis->mInput_FP = aInput_FP;
        aThis->mOutput_FP = (0 == aInput_FP) ? 0 : aThis->mActual();
    }
}

extern void Filter_MD_Tick(Filter_MD* aThis, uint8_t aPeriod_ms)
{
    aThis->mCounter_ms += aPeriod_ms;
    if (aThis->mTable->mPeriod_ms <= aThis->mCounter_ms)
    {
        int32_t lActual_FP = aThis->mActual();
        int16_t lOutput = (int16_t)(aThis->mOutput_FP >> 8);

        aThis->mCounter_ms -= aThis->mTable->mPeriod_ms;

        if (0 <= lOutput)
        {
            int16_t lIndex = lOutput / aThis->mTable->mStep;
            if (aThis->mTable->mLength > lIndex)
            {
                int32_t lDelta_FP;
                int16_t lMax_FP;

                if (aThis->mInput_FP > aThis->mOutput_FP)
                {
                    // Inc.
                    lDelta_FP = aThis->mInput_FP - aThis->mOutput_FP;
                    lMax_FP   = aThis->mTable->mEntries_Inc[lIndex];

                    if (lDelta_FP <= lMax_FP)
                    {
                        aThis->mOutput_FP += lDelta_FP;
                    }
                    else
                    {
                        aThis->mOutput_FP += lMax_FP;
                    }
                }
                else if (aThis->mInput_FP < aThis->mOutput_FP)
                {
                    // Dec.
                    lDelta_FP = aThis->mOutput_FP - aThis->mInput_FP;
                    lMax_FP   = aThis->mTable->mEntries_Dec[lIndex];

                    if (lDelta_FP <= lMax_FP)
                    {
                        aThis->mOutput_FP -= lDelta_FP;
                    }
                    else
                    {
                        aThis->mOutput_FP -= lMax_FP;
                    }
                }
            }
        }
    }
}
