
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/PID_Oven.c

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

#include "PID_Oven.h"

// Configurations
// //////////////////////////////////////////////////////////////////////////

#define DEFAULT_PERIOD_ms (100)

#define OUTPUT_MIN_FP (0)
#define OUTPUT_MAX_FP (2560000)

// Functions
// //////////////////////////////////////////////////////////////////////////

void PID_Oven_Init(PID_Oven* aThis, const PID_Oven_Table* aTable, PID_Oven_InputFunction aSetpoint, PID_Oven_InputFunction aInput)
{
    aThis->mInput    = aInput;
    aThis->mSetpoint = aSetpoint;
    aThis->mTable    = aTable;

    aThis->mP = 0;
    aThis->mI = 0;
    aThis->mD = 0;

    aThis->mPeriod_ms = DEFAULT_PERIOD_ms;

    PID_Oven_Reset(aThis);
}

void PID_Oven_SetParams(PID_Oven* aThis, int32_t aP, int32_t aI, int32_t aD)
{
    aThis->mP = aP;
    aThis->mI = aI;
    aThis->mD = aD;
}

void PID_Oven_Reset(PID_Oven* aThis)
{
    aThis->mCounter_ms    = 0;
    aThis->mError_FP      = 0;
    aThis->mIntegrator_FP = 0;
    aThis->mOutput_FP     = 0;
}

void PID_Oven_Tick(PID_Oven* aThis, uint8_t aPeriod_ms)
{
    aThis->mCounter_ms += aPeriod_ms;
    if (aThis->mPeriod_ms <= aThis->mCounter_ms)
    {
        int32_t lSetpoint_FP = aThis->mSetpoint();
        int32_t lIndex       = lSetpoint_FP / aThis->mTable->mStep_FP;
        int32_t lInput_FP    = aThis->mInput   ();
        int32_t lError_FP    = lSetpoint_FP - lInput_FP;
        int32_t lDelta_FP    = lError_FP - aThis->mError_FP;

        int32_t lP_FP = lError_FP * aThis->mP;
        int32_t lI_FP = lError_FP * aThis->mI;
        int32_t lD_FP = lDelta_FP * aThis->mD;

        int32_t lOffset_FP;
        int32_t lPD_FP;

        aThis->mCounter_ms -= aThis->mPeriod_ms;
        aThis->mError_FP    = lError_FP;
        
        if (0 >= lIndex)
        {
            lOffset_FP = aThis->mTable->mOffsets[0];
        }
        else if (aThis->mTable->mLength <= lIndex)
        {
            lOffset_FP = aThis->mTable->mOffsets[aThis->mTable->mLength - 1];
        }
        else
        {
            lOffset_FP = aThis->mTable->mOffsets[lIndex];
        }

        lPD_FP = lOffset_FP + lP_FP + lD_FP;
        if (OUTPUT_MIN_FP > lPD_FP)
        {
            aThis->mIntegrator_FP = 0;
            aThis->mOutput_FP = OUTPUT_MIN_FP;
        }
        else if (OUTPUT_MAX_FP < lPD_FP)
        {
            aThis->mIntegrator_FP = 0;
            aThis->mOutput_FP = OUTPUT_MAX_FP;
        }
        else
        {
            int32_t lIMax_FP = OUTPUT_MAX_FP - lPD_FP;
            int32_t lIMin_FP = OUTPUT_MIN_FP - lPD_FP;

            aThis->mIntegrator_FP += lI_FP;

            if (lIMin_FP > aThis->mIntegrator_FP)
            {
                aThis->mIntegrator_FP = lIMin_FP;
            }
            else if (lIMax_FP < aThis->mIntegrator_FP)
            {
                aThis->mIntegrator_FP = lIMax_FP;
            }
 
            aThis->mOutput_FP = lPD_FP + aThis->mIntegrator_FP;
        }
    }
}
