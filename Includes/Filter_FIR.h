
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Filter_FIR.h

#pragma once

#ifdef __cplusplus
    extern "C" {
#endif

// Data types
// //////////////////////////////////////////////////////////////////////////

// mSum_FP     (fixed point 24.8)
// mCount
// mPeriod
// mOutput_FP  (fixed point 24.8)
typedef struct Filter_FIR_s
{
    int32_t mSum_FP;
    int16_t mCount;
    int16_t mPeriod;
    int32_t mOutput_FP;
}
Filter_FIR;

// Functions
// //////////////////////////////////////////////////////////////////////////

// aThis
// aToClone
extern void Filter_FIR_Clone(Filter_FIR* aThis, const Filter_FIR* aToClone);

// aThis
// aNewValue_FP  (fixed point 24.8)
extern void Filter_FIR_NewSample(Filter_FIR* aThis, int32_t aNewValue_FP);

// aThis
extern void Filter_FIR_Reset(Filter_FIR* aThis);

// ===== Inline =============================================================

// aThis
inline void Filter_FIR_Init(Filter_FIR* aThis, int16_t aPeriod)
{
    aThis->mPeriod = aPeriod;

    Filter_FIR_Reset(aThis);
}

// aThis
//
// Return  The ouput value (fixed point 24.8)
inline int32_t Filter_FIR_GetOutput_FP(const Filter_FIR* aThis) { return aThis->mOutput_FP; }

#ifdef __cplusplus
    }
#endif
