
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/Filter_SP.c

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
#include "Filter_SP.h"

// Configuration
// //////////////////////////////////////////////////////////////////////////

#define PERIOD_ms (100)

// Constants
// //////////////////////////////////////////////////////////////////////////

// --> OFF --> MAX <==+------------+
//      |       |     |            |
//      +-------+==> SLOPE <--+    |
//                    |       |    |
//                    +-----> ON --+
#define STATE_OFF   (0)
#define STATE_MAX   (1)
#define STATE_SLOPE (2)
#define STATE_ON    (3)

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static void SetState_OFF  (Filter_SP* aThis);
static void SetState_ON   (Filter_SP* aThis);
static void SetState_SLOPE(Filter_SP* aThis, int32_t aOutput_FP);

// Functions
// //////////////////////////////////////////////////////////////////////////

void Filter_SP_Init(Filter_SP* aThis, const Filter_SP_Table* aTable, Filter_SP_InputFunction aActual)
{
    aThis->mActual     = aActual;
    aThis->mCounter_ms = 0;
    aThis->mDelta_FP   = 0;
    aThis->mSlope_FP   = 0;
    aThis->mTable      = aTable;

    SetState_OFF(aThis);
}

void Filter_SP_SetInput(Filter_SP* aThis, int32_t aInput_FP)
{
    if (0 == aInput_FP)
    {
        SetState_OFF(aThis);
    }
    else
    {
        uint8_t lSetOutput = 0;

        switch (aThis->mState)
        {
        case STATE_OFF:
            aThis->mInput_FP = aInput_FP;
            aThis->mState = STATE_MAX;
            lSetOutput = 1;
            break;

        case STATE_MAX:
            if (aThis->mInput_FP != aInput_FP)
            {
                aThis->mInput_FP = aInput_FP;
                lSetOutput = 1;
            }
            break;

        case STATE_SLOPE:
        case STATE_ON:
            if (aThis->mInput_FP != aInput_FP)
            {
                aThis->mInput_FP = aInput_FP;
                aThis->mState = STATE_MAX;
                lSetOutput = 1;
            }
            break;
        }

        if (lSetOutput)
        {
            int32_t                lActual_FP = aThis->mActual();
            const Filter_SP_Table* lTable     = aThis->mTable;

            if (lActual_FP < aInput_FP)
            {
                aThis->mSlope_FP = Table_GetValue(lTable->mSlopes_Inc, aInput_FP);

                aThis->mDelta_FP = aThis->mSlope_FP * 10 * lTable->mDelay_s;

                aThis->mOutput_FP = aInput_FP - aThis->mDelta_FP * 3 / 4;
                if (lActual_FP > aThis->mOutput_FP)
                {
                    SetState_SLOPE(aThis, lActual_FP);
                }
            }
            else if (lActual_FP > aInput_FP)
            {
                aThis->mSlope_FP = Table_GetValue(lTable->mSlopes_Dec, aInput_FP);
                
                aThis->mDelta_FP = aThis->mSlope_FP * 10 * lTable->mDelay_s;

                aThis->mOutput_FP = aInput_FP + aThis->mDelta_FP * 3 / 4;
                if (lActual_FP < aThis->mOutput_FP)
                {
                    SetState_SLOPE(aThis, lActual_FP);
                }
            }
            else
            {
                SetState_ON(aThis);
            }
        }
    }
}

void Filter_SP_Tick(Filter_SP* aThis, uint8_t aPeriod_ms)
{
    aThis->mCounter_ms += aPeriod_ms;
    if (PERIOD_ms <= aThis->mCounter_ms)
    {
        int32_t lActual_FP = aThis->mActual();
        int32_t lTrig_FP;

        aThis->mCounter_ms -= PERIOD_ms;

        switch (aThis->mState)
        {
        case STATE_OFF: break;

        case STATE_MAX:
            if (aThis->mOutput_FP < aThis->mInput_FP)
            {
                lTrig_FP = aThis->mInput_FP - aThis->mDelta_FP;
                if (lActual_FP > lTrig_FP)
                {
                    aThis->mState = STATE_SLOPE;
                }
            }
            else if (aThis->mOutput_FP > aThis->mInput_FP)
            {
                lTrig_FP = aThis->mInput_FP + aThis->mDelta_FP;
                if (lActual_FP < lTrig_FP)
                {
                    aThis->mState = STATE_SLOPE;
                }
            }
            else
            {
                aThis->mState = STATE_ON;
            }
            break;

        case STATE_SLOPE:
            if (aThis->mOutput_FP < aThis->mInput_FP)
            {
                aThis->mOutput_FP += aThis->mSlope_FP;
                if (aThis->mOutput_FP > aThis->mInput_FP)
                {
                    SetState_ON(aThis);
                }
            }
            else if (aThis->mOutput_FP > aThis->mInput_FP)
            {
                aThis->mOutput_FP -= aThis->mSlope_FP;
                if (aThis->mOutput_FP < aThis->mInput_FP)
                {
                    SetState_ON(aThis);
                }
            }
            else
            {
                aThis->mState = STATE_ON;
            }
            break;

        case STATE_ON: break;
        }
    }
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

void SetState_OFF(Filter_SP* aThis)
{
    aThis->mInput_FP = 0;
    aThis->mOutput_FP = 0;
    aThis->mState = STATE_OFF;
}

void SetState_ON(Filter_SP* aThis)
{
    aThis->mOutput_FP = aThis->mInput_FP;
    aThis->mState = STATE_ON;
}

void SetState_SLOPE(Filter_SP* aThis, int32_t aOutput_FP)
{
    aThis->mOutput_FP = aOutput_FP;
    aThis->mState = STATE_SLOPE;
}
