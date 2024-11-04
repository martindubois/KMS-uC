
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Tests/MC56F/MC56F85565/Sources/Test.h

// ==== C ===================================================================
#include <stdint.h>
#include <stdlib.h>

// ==== Includes ============================================================
#include "ADC.h"
#include "Expander.h"
#include "GPIO.h"
#include "I2C.h"
#include "Modbus_Slave.h"
#include "PWM.h"
#include "Tick.h"
#include "Watchdog.h"

// ==== Local ===============================================================
#include "Test.h"

// Configuration
// //////////////////////////////////////////////////////////////////////////

#define _BOARD_NK_E1_CTRL_
// #define _BOARD_UNKNOWN_

// #define _TEST_ADC12_
// #define _TEST_MODBUS_SLAVE_
// #define _TEST_PWMA_
#define _TEST_EXPANDER_

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

static GPIO EXPANDER_INT;
static GPIO EXPANDER_RESET;

static uint8_t EXPANDER_DEFAULT_INPUT[2] = { 0x00, 0x00 };

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

    EXPANDER_INT.mBit           = 10;
    EXPANDER_INT.mPull_Enable   = 1;
    EXPANDER_INT.mPullUp_Select = 1;

    EXPANDER_RESET.mBit      = 8;
    EXPANDER_RESET.mDrive    = 1;
    EXPANDER_RESET.mPushPull = 1;

    MODBUS_OUTPUT_ENABLE.mBit      = 9;
    MODBUS_OUTPUT_ENABLE.mPort     = GPIO_PORT_A;
    MODBUS_OUTPUT_ENABLE.mPushPull = 1;

    MODBUS_RX.mBit  = 8;
    MODBUS_RX.mPort = GPIO_PORT_C;

    MODBUS_TX.mBit           = 7;
    MODBUS_TX.mOutput        = 1;
    MODBUS_TX.mSlewRate_Slow = 1;

    PWMA_0_A.mBit           = 1;
    PWMA_0_A.mOutput        = 1;
    PWMA_0_A.mPushPull      = 1;
    PWMA_0_A.mSlewRate_Slow = 1;

    PWMA_1_A.mBit  = 3;
    PWMA_1_A.mPort = GPIO_PORT_E;

    #ifdef _BOARD_NK_E1_CTRL_
        EXPANDER_INT        .mPort = GPIO_PORT_C;
        EXPANDER_RESET      .mPort = GPIO_PORT_F;
        MODBUS_OUTPUT_ENABLE.mPort = GPIO_PORT_C;
        MODBUS_TX           .mPort = GPIO_PORT_C;
        PWMA_0_A            .mPort = GPIO_PORT_E;
    #endif

    #ifdef _BOARD_UNKNOWN_
        EXPANDER_INT        .mPort = GPIO_PORT_DUMMY;
        EXPANDER_RESET      .mPort = GPIO_PORT_DUMMY;
        MODBUS_OUTPUT_ENABLE.mPort = GPIO_PORT_DUMMY;
        MODBUS_TX           .mPort = GPIO_PORT_DUMMY;
        PWMA_0_A            .mPort = GPIO_PORT_DUMMY;
    #endif

    I2Cs_Init0();
}

void Test_Main()
{
    #ifdef _TEST_ADC12_

        GPIO_InitFunction(ANALOG_0 , 0);
        GPIO_InitFunction(ANALOG_14, 0);

        ADC_Init(ADC_CHANNELS, sizeof(ADC_CHANNELS) / sizeof(ADC_CHANNELS[0]), ADC_INTERRUPT_END_OF_SCAN);

    #endif

    #ifdef _TEST_EXPANDER_

        Expander_Init(1, 0x40, EXPANDER_RESET, EXPANDER_DEFAULT_INPUT, EXPANDER_INT, NULL);

        I2C_Init(1);

    #endif

    #ifdef _TEST_MODBUS_SLAVE_

        GPIO_InitFunction(MODBUS_RX, 1);
        GPIO_InitFunction(MODBUS_TX, 1);

        Modbus_Slave_Init(0, 0x01, MODBUS_RANGES, sizeof(MODBUS_RANGES) / sizeof(MODBUS_RANGES[0]), MODBUS_OUTPUT_ENABLE);

    #endif

    #ifdef _TEST_PWMA_

        GPIO_InitFunction(PWMA_0_A , 0);
        GPIO_InitFunction(PWMA_1_A , 0);

        PWM_Init(0, PWM_MODE_OUTPUT);
        PWM_Init(1, PWM_MODE_CAPTURE_PERIOD);

        PWM_Start(0);
        PWM_Start(1);

        PWM_Set(0, 0, 300);

        PWM_Set2(0, 250, 500);

    #endif

    Tick_Init(80000000);

    Watchdog_Disable();

    for (;;)
    {
        uint16_t lPeriod_ms;

        #ifdef _TEST_MODBUS_SLAVE_
            Modbus_Slave_Work();
        #endif

        lPeriod_ms = Tick_Work();
        if (0 < lPeriod_ms)
        {
            #ifdef _TEST_EXPANDER_
                Expander_Tick(lPeriod_ms);
            #endif

            #ifdef _TEST_MODBUS_SLAVE_
                Modbus_Slave_Tick(lPeriod_ms);
            #endif

            #ifdef _TEST_PWMA_
                PWM_Tick(1, lPeriod_ms);
            #endif
        }
    }
}
