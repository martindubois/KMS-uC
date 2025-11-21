
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024-2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/Filter_IIR.c

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
#include "Filter_IIR.h"

// Functions
// //////////////////////////////////////////////////////////////////////////

void Filter_IIR_Signed_NewSample(Filter_IIR_Signed* aThis, int16_t aNewValue)
{
    // assert(0 < aThis->mN);

    aThis->mAccu -= aThis->mValue;
    aThis->mAccu += aNewValue;
    aThis->mValue = (int16_t)(aThis->mAccu / aThis->mN);
}

void Filter_IIR_Unsigned_NewSample(Filter_IIR_Unsigned* aThis, uint16_t aNewValue)
{
    // assert(0 < aThis->mN);

    aThis->mAccu -= aThis->mValue;
    aThis->mAccu += aNewValue;
    aThis->mValue = (uint16_t)(aThis->mAccu / aThis->mN);
}
