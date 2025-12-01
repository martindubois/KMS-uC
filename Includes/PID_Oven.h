
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/PID_Oven.h

#pragma once

// ===== Local ==============================================================
#ifdef __cplusplus
    extern "C" {
#endif

// Data types
// //////////////////////////////////////////////////////////////////////////

// Return  (fixed point 24.8)
typedef int32_t (*PID_Oven_InputFunction)();

// mOffsets  Pointer to values (fixed point 24.8)
// mStep_FP  (fixed point 24.8)
// mLength
typedef struct
{
    const int32_t* mOffsets;

    int16_t mStep_FP;

    uint8_t mLength;
}
PID_Oven_Table;

// mP              Gain Proportional
// mI              Gain Integrator
// mD              Gain Derivative
// mError_FP       (fixed point 24.8)
// mIntegrator_FP  (fixed point 24.8)
// mOutput_FP      (fixed point 24.8)
// mInput
// mSetpoint
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

    const PID_Oven_Table *mTable;

    PID_Oven_InputFunction mInput;
    PID_Oven_InputFunction mSetpoint;

    uint8_t mCounter_ms;
    uint8_t mPeriod_ms;
}
PID_Oven;

// Functions
// //////////////////////////////////////////////////////////////////////////

// aThis
// aTable     See PID_Oven_Table
// aSetpoint  Function to call to retrieve the filtered consign
// aInput     Function to call to retrieve input
extern void PID_Oven_Init(PID_Oven* aThis, const PID_Oven_Table* aTable, PID_Oven_InputFunction aSetpoint, PID_Oven_InputFunction aInput);

// aThis
// aP     Gain Proportional
// aI     Gain Integrator
// aD     Gain Derivative
extern void PID_Oven_SetParams(PID_Oven* aThis, int32_t aP, int32_t aI, int32_t aD);

// aThis
extern void PID_Oven_Reset(PID_Oven* aThis);

// aThis
// aPeriod_ms  Max = 100 ms
extern void PID_Oven_Tick(PID_Oven* aThis, uint8_t aPeriod_ms);

// ===== Inline =============================================================

// aThis
//
// Returm  (fixed point 24.8)
inline int32_t PID_Oven_GetOutput_FP(const PID_Oven* aThis) { return aThis->mOutput_FP; }

#ifdef __cplusplus
    }
#endif
