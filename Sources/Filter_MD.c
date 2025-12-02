
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
void Filter_MD_SetInput(Filter_MD* aThis, int32_t aInput_FP)
{
    if (aThis->mInput_FP != aInput_FP)
    {
        aThis->mInput_FP = aInput_FP;
        aThis->mOutput_FP = (0 == aInput_FP) ? 0 : aThis->mActual();
    }
}

void Filter_MD_Tick(Filter_MD* aThis, uint8_t aPeriod_ms)
{
    const Filter_MD_Table* lTable = aThis->mTable;

    aThis->mCounter_ms += aPeriod_ms;
    if (lTable->mPeriod_ms <= aThis->mCounter_ms)
    {
        int32_t lInput_FP  = aThis->mInput_FP;
        int32_t lOutput_FP = aThis->mOutput_FP;

        aThis->mCounter_ms -= lTable->mPeriod_ms;

        if (0 <= lOutput_FP)
        {
            int32_t lDelta_FP;
            int16_t lMax_FP;

            if (lInput_FP > lOutput_FP)
            {
                // Inc.
                lDelta_FP = lInput_FP - lOutput_FP;
                lMax_FP   = Table_GetValue(lTable->mMaxDelta_Inc, lOutput_FP);

                aThis->mOutput_FP = lOutput_FP + ((lDelta_FP <= lMax_FP) ? lDelta_FP : lMax_FP);
            }
            else if (lInput_FP < lOutput_FP)
            {
                // Dec.
                lDelta_FP = lOutput_FP - lInput_FP;
                lMax_FP   = Table_GetValue(lTable->mMaxDelta_Dec, lOutput_FP);

                aThis->mOutput_FP = lOutput_FP - ((lDelta_FP <= lMax_FP) ? lDelta_FP : lMax_FP);
            }
        }
    }
}
