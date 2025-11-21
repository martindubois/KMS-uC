
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

// mSum_FP
// mCount
// mOutput_FP
typedef struct Filter_FIR_s
{
    int32_t mSum_FP;
    int16_t mCount;
    int32_t mOutput_FP;
}
Filter_FIR;

// Functions
// //////////////////////////////////////////////////////////////////////////

extern int32_t Filter_FIR_GetOutput_FP(Filter_FIR* aThis);

extern void Filter_FIR_NewSample(Filter_FIR* aThis, int32_t aNewValue_FP);

extern void Filter_FIR_Reset(Filter_FIR* aThis);

// ===== Inline =============================================================

inline void Filter_FIR_Init(Filter_FIR* aThis) { Filter_FIR_Reset(aThis); }

#ifdef __cplusplus
    }
#endif
