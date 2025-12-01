
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/PID.c

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
#include "Filter_MD.h"

#include "PID.h"

// Configurations
// //////////////////////////////////////////////////////////////////////////

#define DEFAULT_PERIOD_ms (100)

#define OUTPUT_MIN_FP (0)
#define OUTPUT_MAX_FP (2560000)

// Functions
// //////////////////////////////////////////////////////////////////////////

void PID_Init(PID* aThis, PID_InputFunction aConsign, PID_InputFunction aInput)
{
    aThis->mConsign = aConsign;
    aThis->mInput   = aInput;

    aThis->mP = 0;
    aThis->mI = 0;
    aThis->mD = 0;

    aThis->mPeriod_ms = DEFAULT_PERIOD_ms;

    PID_Reset(aThis);
}

void PID_SetParams(PID* aThis, int32_t aP, int32_t aI, int32_t aD)
{
    aThis->mP = aP;
    aThis->mI = aI;
    aThis->mD = aD;
}

void PID_Reset(PID* aThis)
{
    aThis->mCounter_ms    = 0;
    aThis->mError_FP      = 0;
    aThis->mIntegrator_FP = 0;
    aThis->mOutput_FP     = 0;
}

void PID_Tick(PID* aThis, uint8_t aPeriod_ms)
{
    aThis->mCounter_ms += aPeriod_ms;
    if (aThis->mPeriod_ms <= aThis->mCounter_ms)
    {
        int32_t lConsign_FP = aThis->mConsign();
        int32_t lInput_FP   = aThis->mInput  ();
        int32_t lError_FP   = lConsign_FP - lInput_FP;
        int32_t lDelta_FP   = lError_FP - aThis->mError_FP;

        int32_t lP_FP = lError_FP * aThis->mP;
        int32_t lI_FP = lError_FP * aThis->mI;
        int32_t lD_FP = lDelta_FP * aThis->mD;

        aThis->mCounter_ms    -= aThis->mPeriod_ms;
        aThis->mError_FP       = lError_FP;
        aThis->mIntegrator_FP += lI_FP;

        if (OUTPUT_MIN_FP > aThis->mIntegrator_FP)
        {
            aThis->mIntegrator_FP = OUTPUT_MIN_FP;
        }
        else if (OUTPUT_MAX_FP < aThis->mIntegrator_FP)
        {
            aThis->mIntegrator_FP = OUTPUT_MAX_FP;
        }

        aThis->mOutput_FP = lP_FP + aThis->mIntegrator_FP + lD_FP;
        if (OUTPUT_MIN_FP > aThis->mOutput_FP)
        {
            aThis->mOutput_FP = OUTPUT_MIN_FP;
        }
        else if (OUTPUT_MAX_FP < aThis->mOutput_FP)
        {
            aThis->mOutput_FP = OUTPUT_MAX_FP;
        }
    }
}
