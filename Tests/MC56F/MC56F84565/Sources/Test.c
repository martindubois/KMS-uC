
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Tests/MC56F/MC56F85565/Sources/Test.h

// ==== C ===================================================================
#include <stdint.h>

// ==== Includes ============================================================
#include "ADC.h"
#include "GPIO.h"

// ==== Local ===============================================================
#include "Test.h"

// Constants
// //////////////////////////////////////////////////////////////////////////

static uint8_t ADC_CHANNELS[] =
{
    0
};

static GPIO ANALOG_0;

// Functions
// //////////////////////////////////////////////////////////////////////////

void Test_Init0()
{
    ANALOG_0.mBit  = 0;
    ANALOG_0.mPort = GPIO_PORT_A;
}

void Test_Main()
{
    GPIO_InitFunction(ANALOG_0);
    
    ADC_Init(ADC_CHANNELS, sizeof(ADC_CHANNELS) / sizeof(ADC_CHANNELS[0]), 0);

    for (;;) {}
}
