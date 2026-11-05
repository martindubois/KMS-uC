
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025-2026 KMS
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

// The following diagram does not show transition occurring when the input
// consign change while the PID is already enabled.
//
//      +--------------------------+
//      |                          |
// --> OFF --> MAX_DEC             |
//     |||      |                  |
//     ||+------+==> SLOPE_DEC     |
//     ||               |          |
//     |+--> MAX_INC    +==> ON <--+
//     |      |         |
//     +------+==> SLOPE_INC
//
// OFF        The filter output is 0.
// ON         The filter output is equal to the input.
// MAX_DEC    The filter output is lower than the actual value and the PID
//            most probably saturate (0 %)
// MAX_INC    The filter output is higher than the actual value and the PID
//            most probably saturate (100 %)
// SLOPE_DEC  The filter output is a ramp to the input consign. The goal is
//            to let the PID match this ramp rather than undershooting the
//            input consign.
// SLOPE_INC  The filter output is a ramp to the input consign. The goal is
//            to let the PID match this ramp rather than overshooting the
//            input consign.
#define STATE_OFF       (0)
#define STATE_MAX_DEC   (1)
#define STATE_MAX_INC   (2)
#define STATE_SLOPE_DEC (3)
#define STATE_SLOPE_INC (4)
#define STATE_ON        (5)

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static void SetState_MAX_DEC  (Filter_SP* aThis, int32_t aOutput_FP);
static void SetState_MAX_INC  (Filter_SP* aThis, int32_t aOutput_FP);
static void SetState_OFF      (Filter_SP* aThis);
static void SetState_ON       (Filter_SP* aThis);
static void SetState_SLOPE_DEC(Filter_SP* aThis, int32_t aOutput_FP);
static void SetState_SLOPE_INC(Filter_SP* aThis, int32_t aOutput_FP);

// Functions
// //////////////////////////////////////////////////////////////////////////

void Filter_SP_Init(Filter_SP* aThis, const Filter_SP_Table* aTable, Filter_SP_InputFunction aActual)
{
    // assert(NULL != aTable);
    // assert(0 < aTable->mDelay_s);
    // assert(NULL != aTable->mSlopes_Dec);
    // assert(NULL != aTable->mSlopes_Inc);
    // assert(NULL != aActual);

    aThis->mActual     = aActual;
    aThis->mCounter_ms = 0;
    aThis->mTable      = aTable;

    SetState_OFF(aThis);
}

void Filter_SP_SetInput(Filter_SP* aThis, int32_t aInput_FP)
{
    // assert(NULL != aThis->mActual);

    if (0 == aInput_FP)
    {
        SetState_OFF(aThis);
    }
    else
    {
        if (aThis->mInput_FP != aInput_FP)
        {
            int32_t                lActual_FP = aThis->mActual();
            int32_t                lOutput_FP;
            const Filter_SP_Table* lTable     = aThis->mTable;

            aThis->mDelta_FP = 10 * lTable->mDelay_s;
            aThis->mInput_FP = aInput_FP;

            if (lActual_FP < aInput_FP)
            {
                aThis->mSlope_FP = Table_GetValue(lTable->mSlopes_Inc, aInput_FP);
                // assert(0 < aThis->mSlope_FP);

                aThis->mDelta_FP *= aThis->mSlope_FP;

                lOutput_FP = aInput_FP - aThis->mDelta_FP * 3 / 4;
                if (lActual_FP >= lOutput_FP)
                {
                    lOutput_FP = lActual_FP + aThis->mSlope_FP;
                    if (lOutput_FP > aThis->mInput_FP)
                    {
                        SetState_ON(aThis);
                    }
                    else
                    {
                        SetState_SLOPE_INC(aThis, lOutput_FP);
                    }
                }
                else
                {
                    SetState_MAX_INC(aThis, lOutput_FP);
                }
            }
            else if (lActual_FP > aInput_FP)
            {
                aThis->mSlope_FP = Table_GetValue(lTable->mSlopes_Dec, aInput_FP);
                // assert(0 < aThis->mSlope_FP);
                
                aThis->mDelta_FP *= aThis->mSlope_FP;

                lOutput_FP = aInput_FP + aThis->mDelta_FP * 3 / 4;
                if (lActual_FP <= lOutput_FP)
                {
                    lOutput_FP = lActual_FP - aThis->mSlope_FP;
                    if (lOutput_FP < aThis->mInput_FP)
                    {
                        SetState_ON(aThis);
                    }
                    else
                    {
                        SetState_SLOPE_DEC(aThis, lOutput_FP);
                    }
                }
                else
                {
                    SetState_MAX_DEC(aThis, lOutput_FP);
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
    // assert(NULL != aThis->mActual);

    aThis->mCounter_ms += aPeriod_ms;
    if (PERIOD_ms <= aThis->mCounter_ms)
    {
        int32_t lActual_FP = aThis->mActual();
        int32_t lTrig_FP;

        aThis->mCounter_ms -= PERIOD_ms;

        switch (aThis->mState)
        {
        case STATE_MAX_DEC:
            // assert(0 < aThis->mDelta_FP);

            lTrig_FP = aThis->mInput_FP + aThis->mDelta_FP;
            if (lActual_FP < lTrig_FP)
            {
                aThis->mState = STATE_SLOPE_DEC;
            }
            break;

        case STATE_MAX_INC:
            // assert(0 < aThis->mDelta_FP);

            lTrig_FP = aThis->mInput_FP - aThis->mDelta_FP;
            if (lActual_FP > lTrig_FP)
            {
                aThis->mState = STATE_SLOPE_INC;
            }
            break;

        case STATE_OFF:
        case STATE_ON:
            break;

        case STATE_SLOPE_DEC:
            // assert(0 < aThis->mSlope_FP);

            aThis->mOutput_FP -= aThis->mSlope_FP;
            if (aThis->mOutput_FP < aThis->mInput_FP)
            {
                SetState_ON(aThis);
            }
            break;

        case STATE_SLOPE_INC:
            // assert(0 < aThis->mSlope_FP);

            aThis->mOutput_FP += aThis->mSlope_FP;
            if (aThis->mOutput_FP > aThis->mInput_FP)
            {
                SetState_ON(aThis);
            }
            break;

        // default: assert(false);
        }
    }
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

void SetState_MAX_DEC(Filter_SP* aThis, int32_t aOutput_FP)
{
    aThis->mOutput_FP = aOutput_FP;
    aThis->mState = STATE_MAX_DEC;
}

void SetState_MAX_INC(Filter_SP* aThis, int32_t aOutput_FP)
{
    aThis->mOutput_FP = aOutput_FP;
    aThis->mState = STATE_MAX_INC;
}

void SetState_OFF(Filter_SP* aThis)
{
    aThis->mDelta_FP = 0;
    aThis->mInput_FP = 0;
    aThis->mOutput_FP = 0;
    aThis->mSlope_FP = 0;
    aThis->mState = STATE_OFF;
}

void SetState_ON(Filter_SP* aThis)
{
    aThis->mDelta_FP = 0;
    aThis->mOutput_FP = aThis->mInput_FP;
    aThis->mSlope_FP = 0;
    aThis->mState = STATE_ON;
}

void SetState_SLOPE_DEC(Filter_SP* aThis, int32_t aOutput_FP)
{
    aThis->mDelta_FP = 0;
    aThis->mOutput_FP = aOutput_FP;
    aThis->mState = STATE_SLOPE_DEC;
}

void SetState_SLOPE_INC(Filter_SP* aThis, int32_t aOutput_FP)
{
    aThis->mDelta_FP = 0;
    aThis->mOutput_FP = aOutput_FP;
    aThis->mState = STATE_SLOPE_INC;
}
