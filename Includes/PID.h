
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/PID.h

#pragma once

// ===== Local ==============================================================
struct Filter_MD_s;

#ifdef __cplusplus
    extern "C" {
#endif

// Data types
// //////////////////////////////////////////////////////////////////////////

// mP
// mI
// mD
// mIntegrator
// mLastInput
// mOutput
// mConsign
// mInput
// mCounter_ms
typedef struct
{
    int32_t mP;
    int32_t mI;
    int32_t mD;

    int32_t mError_FP;
    int32_t mIntegrator_FP;
    int32_t mOutput_FP;

    struct Filter_MD_s * mConsign;
    struct Filter_FIR_s* mInput;

    uint8_t mCounter_ms;
    uint8_t mPeriod_ms;
}
PID;

// Functions
// //////////////////////////////////////////////////////////////////////////

extern void PID_Init(PID* aThis, struct Filter_MD_s* aConsign, struct Filter_FIR_s* aInput);

extern void PID_SetParams(PID* aThis, int32_t aP, int32_t aI, int32_t aD);

extern void PID_Reset(PID* aThis);

extern void PID_Tick(PID* aThis, uint8_t aPeriod_ms);

// ===== Inline =============================================================

inline int32_t PID_GetOutput_FP(const PID* aThis) { return aThis->mOutput_FP; }

#ifdef __cplusplus
    }
#endif
