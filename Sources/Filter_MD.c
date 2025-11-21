
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

void Filter_MD_Init(Filter_MD* aThis, const Filter_MD_Table* aTable)
{
    aThis->mTable = aTable;

    Filter_MD_Reset(aThis, 0);
}

extern void Filter_MD_Reset(Filter_MD* aThis, int32_t aInput_FP)
{
    aThis->mCounter_ms = 0;
    aThis->mInput_FP   = aInput_FP;
    aThis->mOutput_FP  = aInput_FP;
}

extern void Filter_MD_Tick(Filter_MD* aThis, uint8_t aPeriod_ms)
{
    aThis->mCounter_ms += aPeriod_ms;
    if (aThis->mTable->mPeriod_ms <= aThis->mCounter_ms)
    {
        int16_t lOutput = aThis->mOutput_FP >> 8;

        aThis->mCounter_ms -= aThis->mTable->mPeriod_ms;

        if (0 <= lOutput)
        {
            int16_t lIndex = lOutput / aThis->mTable->mStep;
            if (aThis->mTable->mLength > lIndex)
            {
                uint32_t lDelta_FP;
                uint16_t lMax_FP;

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
