
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Expander.h

#pragma once

// ===== Local ==============================================================
#include "GPIO.h"

// Data type
// //////////////////////////////////////////////////////////////////////////

typedef void (*Expander_Callback)();

// Functions
// //////////////////////////////////////////////////////////////////////////

// aI2C             The I2C bus index
// aDevice          The device's I2C address
// aReset           The GPIO used to reset the expanded if a communication
//                  error occure. This function calls GPIO_Init for this
//                  GPIO.
//                      .mBit
//                      .mDrive
//                      .mInterrupt_Falling
//                      .mOutput            : Ignored, must be an output
//                      .mPort              : See GPIO_PORT_...
//                      .mPull_Enable
//                      .mPullUp_Select
//                      .mPushPull
//                      .mSlewRate_Slow     : Ignored, must be set
// aDefaultInput    Simulated input when communication is not yet established
// aInt             The GPIO used to get interrupt from the expander. This
//                  function calls GPIO_Init for this GPIO.
//                      .mBit
//                      .mDrive             : Ignored, because this is an input
//                      .mInterrupt_Falling : Ignored, must be set
//                      .mOutput            : Ignored, must be an input
//                      .mPort              : See GPIO_PORT_..., do not use GPIO_PORT_DUMMY
//                      .mPull_Enable
//                      .mPullUp_Select
//                      .mPushPull          : Ignored, because this is an input
//                      .mSlewRate_Slow     : Ignored, because this is an input
// aOnInputChanged  Optional
extern void Expander_Init(uint8_t aI2C, uint8_t aDevice, GPIO aReset, const uint8_t* aDefaultInput, GPIO aInt, Expander_Callback aOnInputChanged);

// aDesc
//            .mBit               : 0 to 7
//            .mDrive
//            .mInterrupt_Falling : Ignored
//            .mOutput
//            .mPort              : GPIO_PORT_A, GPIO_PORT_B or GPIO_PORT_DUMMY
//            .mPull_Enale
//            .mPullUp_Select
//            .mPushPull          : Apply to all bit of the same port
//            .mSlewRate_Slow     : Ignored
extern void Expander_GPIO_Init(GPIO aDesc);

// Return  false
//         true
extern uint8_t Expander_GPIO_Input(GPIO aDesc);

// aValue  false
//         true
extern void Expander_GPIO_Output(GPIO aDesc, uint8_t aValue);

// Return  false
//         true
extern uint8_t Expander_GPIO_Output_Get(GPIO aDesc);

extern void Expander_Tick(uint16_t aPeriod_ms);
