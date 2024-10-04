
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
#include "PWM.h"
#include "Tick.h"

// ==== Local ===============================================================
#include "Test.h"

// Constants
// //////////////////////////////////////////////////////////////////////////

static uint8_t ADC_CHANNELS[] =
{
    0
};

static GPIO ANALOG_0;

static GPIO PWMA_0_A;
static GPIO PWMA_1_A;

// Entry point
// //////////////////////////////////////////////////////////////////////////

void ADC12_Interrupt_CC0();

#pragma interrupt alignsp saveall
void ADC12_Interrupt_CC0()
{
    ADC_AcknowledgeInterrupt();
}

// Functions
// //////////////////////////////////////////////////////////////////////////

void Test_Init0()
{
    ANALOG_0.mBit  = 0;
    ANALOG_0.mPort = GPIO_PORT_A;

    PWMA_0_A.mBit           = 1;
    PWMA_0_A.mPort          = GPIO_PORT_E;
    PWMA_0_A.mOutput        = 1;
    PWMA_0_A.mPushPull      = 1;
    PWMA_0_A.mSlewRate_Slow = 1;

    PWMA_1_A.mBit      = 3;
    PWMA_1_A.mPort     = GPIO_PORT_E;
}

void Test_Main()
{
    GPIO_InitFunction(ANALOG_0);
    GPIO_InitFunction(PWMA_0_A);
    GPIO_InitFunction(PWMA_1_A);
    
    ADC_Init(ADC_CHANNELS, sizeof(ADC_CHANNELS) / sizeof(ADC_CHANNELS[0]), ADC_INTERRUPT_END_OF_SCAN);

    PWM_Init(0, PWM_MODE_OUTPUT);
    PWM_Init(1, PWM_MODE_CAPTURE_PERIOD);

    Tick_Init(80000000);

    PWM_Start(0);
    PWM_Start(1);

    PWM_Set(0, 250, 500);

    for (;;)
    {
        uint16_t lPeriod_ms = Tick_Work();
        if (0 < lPeriod_ms)
        {
            PWM_Tick(1, lPeriod_ms);
        }
    }
}
