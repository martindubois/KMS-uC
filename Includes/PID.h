
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/PID.h

#pragma once

// ===== Local ==============================================================
#ifdef __cplusplus
    extern "C" {
#endif

// Data types
// //////////////////////////////////////////////////////////////////////////

// Return  (fixed point 24.8)
typedef int32_t (*PID_InputFunction)();

// mP              Gain Proportional
// mI              Gain Integrator
// mD              Gain Derivative
// mError_FP       (fixed point 24.8)
// mIntegrator_FP  (fixed point 24.8)
// mOutput_FP      (fixed point 24.8)
// mConsign
// mInput
// mCounter_ms
// mPeriod_ms      Default = 100 ms, Max = 100 ms
typedef struct
{
    int32_t mP;
    int32_t mI;
    int32_t mD;

    int32_t mError_FP;
    int32_t mIntegrator_FP;
    int32_t mOutput_FP;

    PID_InputFunction mConsign;
    PID_InputFunction mInput;

    uint8_t mCounter_ms;
    uint8_t mPeriod_ms;
}
PID;

// Functions
// //////////////////////////////////////////////////////////////////////////

// aThis
// aConsign  Function to call to retrieve the filtered consign
// aInput    Function to call to retrieve input
extern void PID_Init(PID* aThis, PID_InputFunction aConsign, PID_InputFunction aInput);

// aThis
// aP     Gain Proportional
// aI     Gain Integrator
// aD     Gain Derivative
extern void PID_SetParams(PID* aThis, int32_t aP, int32_t aI, int32_t aD);

// aThis
extern void PID_Reset(PID* aThis);

// aThis
// aPeriod_ms  Max = 100 ms
extern void PID_Tick(PID* aThis, uint8_t aPeriod_ms);

// ===== Inline =============================================================

// aThis
//
// Returm  (fixed point 24.8)
inline int32_t PID_GetOutput_FP(const PID* aThis) { return aThis->mOutput_FP; }

#ifdef __cplusplus
    }
#endif
