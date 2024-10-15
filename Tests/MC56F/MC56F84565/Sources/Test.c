
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
#include "Modbus_Slave.h"
#include "PWM.h"
#include "Tick.h"

// ==== Local ===============================================================
#include "Test.h"

// Configuration
// //////////////////////////////////////////////////////////////////////////

#define _BOARD_NK_E1_CTRL_
// #define _BOARD_UNKNOWN_

// Variables
// //////////////////////////////////////////////////////////////////////////

static uint16_t sModbus_Data[1];

// Constants
// //////////////////////////////////////////////////////////////////////////

static uint8_t ADC_CHANNELS[] =
{
    0  | ADC_SIGNED,
    14 | ADC_SIGNED,
};

static GPIO ANALOG_0;
static GPIO ANALOG_14;

static GPIO MODBUS_OUTPUT_ENABLE;
static GPIO MODBUS_RX;
static GPIO MODBUS_TX;

static Modbus_Slave_Range MODBUS_RANGES[] =
{
    { 0, 0, sizeof(sModbus_Data) / sizeof(uint16_t), sModbus_Data, Modbus_Slave_Callback_Default, Modbus_Slave_Callback_Default, Modbus_Slave_Callback_Default }
};

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

    ANALOG_14.mBit  = 6;
    ANALOG_14.mPort = GPIO_PORT_B;

    MODBUS_OUTPUT_ENABLE.mBit           = 9;
    MODBUS_OUTPUT_ENABLE.mOutput        = 1;
    MODBUS_OUTPUT_ENABLE.mPushPull      = 1;
    MODBUS_OUTPUT_ENABLE.mSlewRate_Slow = 1;

    MODBUS_RX.mBit      = 8;
    MODBUS_RX.mPort     = GPIO_PORT_C;
    MODBUS_RX.mFunction = 1;

    MODBUS_TX.mBit           = 7;
    MODBUS_TX.mFunction      = 1;
    MODBUS_TX.mOutput        = 1;
    MODBUS_TX.mSlewRate_Slow = 1;

    PWMA_0_A.mBit           = 1;
    PWMA_0_A.mOutput        = 1;
    PWMA_0_A.mPushPull      = 1;
    PWMA_0_A.mSlewRate_Slow = 1;

    PWMA_1_A.mBit  = 3;
    PWMA_1_A.mPort = GPIO_PORT_E;

    #ifdef _BOARD_NK_E1_CTRL_
        MODBUS_OUTPUT_ENABLE.mPort = GPIO_PORT_C;
        MODBUS_TX           .mPort = GPIO_PORT_C;
        PWMA_0_A            .mPort = GPIO_PORT_E;
    #endif

    #ifdef _BOARD_UNKNOWN_
        MODBUS_OUTPUT_ENABLE.mPort = GPIO_PORT_DUMMY;
        MODBUS_TX           .mPort = GPIO_PORT_DUMMY;
        PWMA_0_A            .mPort = GPIO_PORT_DUMMY;
    #endif
}

void Test_Main()
{
    GPIO_InitFunction(ANALOG_0);
    GPIO_InitFunction(MODBUS_RX);
    GPIO_InitFunction(MODBUS_TX);
    GPIO_InitFunction(PWMA_0_A);
    GPIO_InitFunction(PWMA_1_A);
    
    ADC_Init(ADC_CHANNELS, sizeof(ADC_CHANNELS) / sizeof(ADC_CHANNELS[0]), ADC_INTERRUPT_END_OF_SCAN);

    Modbus_Slave_Init(0, 0x01, MODBUS_RANGES, sizeof(MODBUS_RANGES) / sizeof(MODBUS_RANGES[0]), MODBUS_OUTPUT_ENABLE);

    PWM_Init(0, PWM_MODE_OUTPUT);
    PWM_Init(1, PWM_MODE_CAPTURE_PERIOD);

    Tick_Init(80000000);

    PWM_Start(0);
    PWM_Start(1);

    PWM_Set(0, 0, 300);

    PWM_Set2(0, 250, 500);

    for (;;)
    {
        uint16_t lPeriod_ms;

        Modbus_Slave_Work();

        lPeriod_ms = Tick_Work();
        if (0 < lPeriod_ms)
        {
            Modbus_Slave_Tick(lPeriod_ms);
            PWM_Tick(1, lPeriod_ms);
        }
    }
}
