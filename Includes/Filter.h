
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Filter.h

#pragma once

// Data types
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    int32_t  mAccu;
    int16_t  mValue;
    uint16_t mN;
}
Filter_IIR_Signed;

typedef struct
{
    uint32_t mAccu;
    uint16_t mValue;
    uint16_t mN;
}
Filter_IIR_Unsigned;

// Functions
// //////////////////////////////////////////////////////////////////////////

// How to compute N
//
// Fc = Fs / (0.999 * N - 0.6)
//
// N = ( Fs / Fc + 0.6 ) / 0.999
//
// Where Fc is Cutoff frequency, Fs is sampling frequency
#define Filter_IIR_Init(T, N) ((T)->mN = (N))

#define Filter_IIR_GetValue(T) ((T)->mValue)

extern void Filter_IIR_Signed_NewSample(Filter_IIR_Signed* aThis, int16_t aNewValue);

extern void Filter_IIR_Unsigned_NewSample(Filter_IIR_Unsigned* aThis, uint16_t aNewValue);
