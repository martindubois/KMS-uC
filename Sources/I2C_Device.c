
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/I2C_Device.c

// References
// //////////////////////////////////////////////////////////////////////////

// Assumptions
// //////////////////////////////////////////////////////////////////////////

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////

// Code
// //////////////////////////////////////////////////////////////////////////

// ===== C ==================================================================
#include <stdint.h>

// ===== Includes ===========================================================
#include "I2C_Device.h"

// Functions
// //////////////////////////////////////////////////////////////////////////

void I2C_Device_Init(I2C_Device* aThis, uint8_t aBusIndex, uint8_t aAddress)
{
    aThis->mAddress  = aAddress;
    aThis->mBusIndex = aBusIndex;
}
