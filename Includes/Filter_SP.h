
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Filter_SP.h

// The filter SP (for Set Point) is intended to filter a PID setpoint.

#pragma once

// ===== Includes ===========================================================
#include "Table.h"

#ifdef __cplusplus
    extern "C" {
#endif

// Data types
// //////////////////////////////////////////////////////////////////////////

// Return  (fixed point 24.8)
typedef int32_t (*Filter_SP_InputFunction)();

// mSlopes_Dec  See Table
// mSlopes_Inc  See Table
// mDelay_s
typedef struct
{
    const Table* mSlopes_Dec;
    const Table* mSlopes_Inc;

    uint8_t mDelay_s;
}
Filter_SP_Table;

// mDelta_FP    (fixed point 8.8)
// mInput_FP    (fixed point 24.8)
// mOutput_FP   (fixed point 24.8)
// mActual      See Filter_MD_InputFunction
// mSlope_FP    (fixed point 8.8)
// mCounter_ms  Time since the last iteration
typedef struct
{
    int32_t mDelta_FP;
    int32_t mInput_FP;
    int32_t mOutput_FP;

    const Filter_SP_Table* mTable;

    Filter_SP_InputFunction mActual;

    int16_t mSlope_FP;

    uint8_t mCounter_ms;
    uint8_t mState;
}
Filter_SP;

// Functions
// //////////////////////////////////////////////////////////////////////////

// aThis
// aTable     See Filter_SP_Table
// aActual    See Filter_SP_InputFunction
extern void Filter_SP_Init(Filter_SP* aThis, const Filter_SP_Table* aTable, Filter_SP_InputFunction aActual);

// aThis
// aInput_FP  (fixed point 24.8)
extern void Filter_SP_SetInput(Filter_SP* aThis, int32_t aInput_FP);

// aThis
// aPeriod_ms  Time since the last call
extern void Filter_SP_Tick(Filter_SP* aThis, uint8_t aPeriod_ms);

// ===== Inline =============================================================

// aThis
//
// Return  (fixed point 24.8)
inline int32_t Filter_SP_GetInput_FP(const Filter_SP* aThis) { return aThis->mInput_FP; }

// aThis
//
// Return  (fixed point 24.8)
inline int32_t Filter_SP_GetOutput_FP(const Filter_SP* aThis) { return aThis->mOutput_FP; }

#ifdef __cplusplus
    }
#endif
