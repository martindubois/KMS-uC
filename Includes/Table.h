
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2025 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Table.h

#pragma once

#ifdef __cplusplus
    extern "C" {
#endif

// Data types
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    const int16_t* mValues;

    int16_t mStep_FP;

    int8_t mLength;
}
Table;

// Functions
// //////////////////////////////////////////////////////////////////////////

extern int16_t Table_GetValue(const Table* aThis, int32_t aInput_FP);

#ifdef __cplusplus
    }
#endif
