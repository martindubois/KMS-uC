
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
// aInt             The GPIO used to get interrupt from the expander
// aOnInputChanged  Optional
extern void Expander_Init(uint8_t aI2C, uint8_t aDevice, GPIO aInt, Expander_Callback aOnInputChanged);

// aDesc.mBit               : 0 to 7
//      .mFunction          : Ignored
//      .mInterrupt_Falling : Ignored
//      .mPort              : GPIO_PORT_A or GPIO_PORT_B
//      .mPushPull          : Apply to all bit of the same port
//      .mSlewRate_Slow     : Ignored
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
