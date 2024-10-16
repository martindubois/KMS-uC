
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Debounced.h

#pragma once

// Data type
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    volatile uint16_t* mRegister;

    uint16_t mMask;

    uint8_t  mCounter;
    uint8_t  mValue;
}
Debounced;

// Functions
// //////////////////////////////////////////////////////////////////////////

// To use with a GPIO, use the GPIO_GetRegisterAndMask function to retrieve
// the register address and the mask.
//
// aRegister  The address of the register to read
// aMask      The bit to look at
extern void Debounced_Init(Debounced* aThis, volatile uint16_t* aRegister, uint16_t aMask);

// Return  false
//         true
extern uint8_t Debounced_GetValue(Debounced* aThis);
