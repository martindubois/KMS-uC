
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Filter_MD.h

// The filter MD (for Max Delta) is intended to filter a PID consign.

#pragma once

#ifdef __cplusplus
    extern "C" {
#endif

// Data types
// //////////////////////////////////////////////////////////////////////////

// Return  (fixed point 24.8)
typedef int32_t (*Filter_MD_InputFunction)();

// mEntries_Dec  (fixed point 8.8)
// mEntries_Inc  (fixed point 8.8)
// mStep         Distance between table entries
// mLength       Number of table entries
// mPeriod_ms    Time between iteration. Max. = 100 ms
typedef struct
{
    const int16_t* mEntries_Dec;
    const int16_t* mEntries_Inc;

    int16_t mStep;

    uint8_t mLength;
    uint8_t mPeriod_ms;
}
Filter_MD_Table;

// mInput_FP    (fixed point 24.8)
// mOutput_FP   (fixed point 24.8)
// mActual      See Filter_MD_InputFunction
// mTable       See Filter_MD_Table
// mCounter_ms  Time since the last iteration
typedef struct Filter_MD_s
{
    int32_t mInput_FP;
    int32_t mOutput_FP;

    Filter_MD_InputFunction mActual;

    const Filter_MD_Table* mTable;

    uint8_t mCounter_ms;
}
Filter_MD;

// Functions
// //////////////////////////////////////////////////////////////////////////

// aThis    See Filter_MD
// aTable   See Filter_MD_Table
// aActual  Function to call to retrieve actual controlled variable value
extern void Filter_MD_Init(Filter_MD* aThis, const Filter_MD_Table* aTable, Filter_MD_InputFunction aActual);

// aThis      See Filter_MD
// aInput_FP  The value 0 disable the filter. If the value is equal to the
//            already set value, this function do nothing. If the value
//            change, the ouput is reset to the actual value.
//            (fixed point 24.8)
extern void Filter_MD_SetInput(Filter_MD* aThis, int32_t aInput_FP);

// aThis       See Filter_MD
// aPeriod_ms  Time since the last tick. Max. = 100 ms
extern void Filter_MD_Tick(Filter_MD* aThis, uint8_t aPeriod_ms);

// ===== Inline =============================================================

// aThis  See Filter_MD
//
// Return  Input value (fixed point 24.8)
inline int32_t Filter_MD_GetInput_FP(const Filter_MD* aThis) { return aThis->mInput_FP; }

// aThis  See Filter_MD
//
// Return  Output value (fixed point 24.8)
inline int32_t Filter_MD_GetOutput_FP(const Filter_MD* aThis) { return aThis->mOutput_FP; }

#ifdef __cplusplus
    }
#endif
