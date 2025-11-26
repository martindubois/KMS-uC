
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Filter_MD.h

#pragma once

#ifdef __cplusplus
    extern "C" {
#endif

// Data types
// //////////////////////////////////////////////////////////////////////////

// mEntries    Fixed point 8.8
// mStep
// mLength
// mPeriod_ms  Max. = 100 ms
typedef struct
{
    const int16_t* mEntries_Dec;
    const int16_t* mEntries_Inc;

    int16_t mStep;

    uint8_t mLength;
    uint8_t mPeriod_ms;
}
Filter_MD_Table;

// mInput
// mOutput
// mTable
// mCounter_ms
typedef struct Filter_MD_s
{
    int32_t mInput_FP;
    int32_t mOutput_FP;

    const Filter_MD_Table* mTable;

    uint8_t mCounter_ms;
}
Filter_MD;

// Functions
// //////////////////////////////////////////////////////////////////////////

// aThis
// aInput
// aTable
extern void Filter_MD_Init(Filter_MD* aThis, const Filter_MD_Table* aTable);

// aThis
// aInput
extern void Filter_MD_Reset(Filter_MD* aThis, int32_t aInput_FP);

// aThis
// aPeriod_ms  Max. = 100 ms
extern void Filter_MD_Tick(Filter_MD* aThis, uint8_t aPeriod_ms);

// ===== Inline =============================================================

inline int32_t Filter_MD_GetInput_FP(const Filter_MD* aThis) { return aThis->mInput_FP; }

inline int32_t Filter_MD_GetOutput_FP(const Filter_MD* aThis) { return aThis->mOutput_FP; }

inline void Filter_MD_SetInput(Filter_MD* aThis, int32_t aInput_FP) { aThis->mInput_FP = aInput_FP; }

#ifdef __cplusplus
    }
#endif
