
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/MC56F/PWMA.c

// Assumption
// //////////////////////////////////////////////////////////////////////////
//
// - The system clock (IP Bus) is 80 MHz

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////
//
// Project configuration
// - Define _MC56F84565_
//
// Processor Expert Configuration
// - Add an InterruptVector for INT_eFlexPWMA_CAP associated to
//   PWMA_Interrupt_CAP function

// Code
// //////////////////////////////////////////////////////////////////////////
//
// - Configure Input/Output pin using GPIO_InitFunction

// ===== C ==================================================================
#include <stdint.h>

// ===== Includes ===========================================================
#include "MC56F_SIM.h"

#include "PWM.h"

// Data types
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint16_t mCounter;
    uint16_t mInitialCount;
    uint16_t mControl2;
    uint16_t mControl;
    uint16_t mReserved0;
    uint16_t mValue0;
    uint16_t mReserved1;
    uint16_t mValue1;
    uint16_t mReserved2;
    uint16_t mValue2;
    uint16_t mReserved3;
    uint16_t mValue3;
    uint16_t mReserved4;
    uint16_t mValue4;
    uint16_t mReserved5;
    uint16_t mValue5;
    uint16_t mReserved6;
    uint16_t mOutputControl;
    uint16_t mStatus;
    uint16_t mInterruptEnable;
    uint16_t mDMAEnable;
    uint16_t mOutputTriggerControl;
    uint16_t mFaultDisableMapping0;
    uint16_t mFaultDisableMapping1;
    uint16_t mDeadTime0;
    uint16_t mDeadTime1;
    uint16_t mCaptureControlA;
    uint16_t mCaptureCompareA;
    uint16_t mCaptureControlB;
    uint16_t mCaptureCompareB;
    uint16_t mCaptureControlX;
    uint16_t mCaptureCompareX;
    uint16_t mCaptureValue0;
    uint16_t mCaptureValueCycle0;
    uint16_t mCaptureValue1;
    uint16_t mCaptureValueCycle1;
    uint16_t mCaptureValue2;
    uint16_t mCaptureValueCycle2;
    uint16_t mCaptureValue3;
    uint16_t mCaptureValueCycle3;
    uint16_t mCaptureValue4;
    uint16_t mCaptureValueCycle4;
    uint16_t mCaptureValue5;
    uint16_t mCaptureValueCycle5;
    uint16_t mReserved7[4];
}
ChannelRegs;

typedef struct
{
    uint16_t mOutputEnable;
    uint16_t mMask;
    uint16_t mSoftwareControlledOutput;
    uint16_t mSourceSelect;
    uint16_t mMasterControl;
    uint16_t mFaultControl0;
    uint16_t mFaultStatus0;
    uint16_t mFaultFilter0;
    uint16_t mFaultTest0;
    uint16_t mFaultControl1;
    uint16_t mFaultControl20;
    uint16_t mFaultStatus1;
    uint16_t mFaultFilter1;
    uint16_t mFaultTest1;
    uint16_t mFaultControl21;
}
CommonRegs;

#define MCTRL_LDOK0  (0x0001)
#define MCTRL_CLDOK0 (0x0010)
#define MCTRL_RUN0   (0x0100)

#define INPUT_OUTPUT_QTY (2)

typedef struct
{
    uint16_t mCaptures   [INPUT_OUTPUT_QTY];
    uint16_t mTimeouts_ms[INPUT_OUTPUT_QTY];
    uint16_t mValues     [INPUT_OUTPUT_QTY];
    uint8_t  mStates     [INPUT_OUTPUT_QTY];
    uint8_t  mIndex;
    uint8_t  mMode;
}
Context;

// --> UNKNOWN --> WAIT <-------+
//     |           | |          |
//     |           | +--> KNOWN |
//     |           |      |     |
//     +-----------+======+==> ERROR
#define STATE_ERROR   (0)
#define STATE_KNOWN   (1)
#define STATE_UNKNOWN (2)
#define STATE_WAIT    (3)

// Constants
// //////////////////////////////////////////////////////////////////////////

#define PWMA_QTY (4)

static volatile CommonRegs* COMMON_REGS = (CommonRegs*)0x0000e6c0;

static volatile ChannelRegs* CHANNEL_REGS = (ChannelRegs*)0x0000e600;

static const uint16_t SIM_PCE3_BITS[PWMA_QTY] = { 0x0080, 0x0040, 0x0020, 0x0010 };

#define TIMEOUT_ms (200)

// Variables
// //////////////////////////////////////////////////////////////////////////

static Context sContexts[PWMA_QTY];

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static void Poll(Context* aThis);

static void ReadCapture(Context* aThis);

// Entry points
// //////////////////////////////////////////////////////////////////////////

void PWMA_Interrupt_CAP();

#pragma interrupt alignsp saveall
void PWMA_Interrupt_CAP()
{
    unsigned int i;

    for (i = 0; i < PWMA_QTY; i++)
    {
        ReadCapture(sContexts + i);
    }
}

// Functions
// //////////////////////////////////////////////////////////////////////////

void PWM_Init(uint8_t aIndex, uint8_t aMode)
{
    // assert(PWMA_QTY > aIndex);

    uint16_t              lMasterControl;
    volatile ChannelRegs* lR    = CHANNEL_REGS + aIndex;
    Context             * lThis = sContexts + aIndex;

    uint8_t i;

    *SIM_PCE3 |= SIM_PCE3_BITS[aIndex];

    for (i = 0; i < INPUT_OUTPUT_QTY; i++)
    {
        lThis->mCaptures   [i] = 0;
        lThis->mStates     [i] = STATE_UNKNOWN;
        lThis->mTimeouts_ms[i] = 0;
    }

    lThis->mIndex = aIndex;
    lThis->mMode  = aMode;

    lMasterControl = COMMON_REGS->mMasterControl;

    COMMON_REGS->mMasterControl = lMasterControl | (MCTRL_CLDOK0 << aIndex);

    // COMMON_REGS->mFaultControl0 = 0; // Default E1
    // COMMON_REGS->mFaultControl1 = 0; // Default E1

    // lR->mDMAEnable            = 0; // Default
    // lR->mInitialCount         = 0; // Default E1
    // lR->mOutputTriggerControl = 0; // Default E1
    // lR->mValue0               = 0; // Default E1
    // lR->mValue2               = 0; // Default E1
    // lR->mValue4               = 0; // Default E1

    switch (aMode)
    {
    case PWM_MODE_CAPTURE_PERIOD:
        lR->mInterruptEnable = 0x0500; // CA01E, CB01E
        // lR->mOutputControl   = 0x0000; // Default
        lR->mControl2        = 0x2000; // INDEP
        lR->mControl         = 0x0470; // FULL, PRSC = 7
        lR->mValue1          = 0xffff;
        // lR->mValue3          = 0; // Default E1
        // lR->mValue5          = 0; // Default E1
        lR->mCaptureControlA = 0x0009; // EDGA0 = Capture Rising edges, ARMA
        lR->mCaptureControlB = 0x0009; // EDGB0 = Capture Rising edges, ARMB
        break;

    case PWM_MODE_OUTPUT:
        // lR->mInterruptEnable = 0x0000; // Default E1
        lR->mOutputControl   = 0x0614; // POLA, POLB, PWMBFS = 1, PWMAFS = 1
        lR->mControl2        = 0x3800; // INDEP, PWM23_INIT, PWM45_INIT
        lR->mControl         = 0x0400; // FULL - Default
        lR->mValue1          = 4000; // 80 MHz -> T = 12.5 us, 4000 * 12.5 us = 50 ms
        lR->mValue3          = 2000;
        lR->mValue5          = 2000;
        // lR->mCaptureControlA = 0; // Default
        // lR->mCaptureControlB = 0; // Default

        COMMON_REGS->mOutputEnable |= 0x0110 << aIndex;
        break;
    }

    lR->mFaultDisableMapping0 = 0xf000;
    lR->mFaultDisableMapping1 = 0xf000;
    // lR->mDeadTime0            = 2047; // Default E1
    // lR->mDeadTime1            = 2047; // Default E1
    // lR->mCaptureCompareA      = 0; // Default
    // lR->mCaptureCompareB      = 0; // Default
    // lR->mCaptureCompareX      = 0; // Default
    // lR->mCaptureControlX      = 0; // Default

    // COMMON_REGS->mFaultFilter0   = 0; // Default E1
    // COMMON_REGS->mFaultFilter1   = 0; // Default E1
    // COMMON_REGS->mMask           = 0; // Default E1
    // COMMON_REGS->mSoftwareControlledOutput = 0; // Default E1
    // COMMON_REGS->mSourceSelect   = 0; // Default E1

    COMMON_REGS->mMasterControl  = lMasterControl | (MCTRL_LDOK0 << aIndex);
    COMMON_REGS->mFaultStatus0   = 0xf0ff;
    COMMON_REGS->mFaultStatus1   = 0xf0ff;
    // COMMON_REGS->mFaultControl20 = 0; // Default E1
    // COMMON_REGS->mFaultControl21 = 0; // Default E1
    // COMMON_REGS->mFaultControl0  = 0; // Default E1
    // COMMON_REGS->mFaultControl1  = 0; // Default E1
}

void PWM_Set(uint8_t aIndex, uint16_t aDutyCycleA, uint16_t aDutyCycleB)
{
    // assert(PWMA_QTY > aIndex);

    uint16_t              lMasterControl;
    volatile ChannelRegs* lR = CHANNEL_REGS + aIndex;

    lMasterControl = COMMON_REGS->mMasterControl;

    COMMON_REGS->mMasterControl = lMasterControl | (MCTRL_CLDOK0 << aIndex);

    lR->mValue3 = aDutyCycleA * 4;
    lR->mValue5 = aDutyCycleB * 4;

    lMasterControl = COMMON_REGS->mMasterControl;

    COMMON_REGS->mMasterControl = lMasterControl | (MCTRL_LDOK0 << aIndex);
}

uint32_t PWM_Read(uint8_t aIndex, uint8_t aInput)
{
    // assert(PWMA_QTY > aIndex);

    Context* lThis = sContexts + aIndex;

    uint32_t lResult_us = PWM_ERROR;

    if (STATE_KNOWN == lThis->mStates[aInput])
    {
        lResult_us = lThis->mValues[aInput];

        lResult_us *= 256;
        lResult_us /= 100;
    }

    return lResult_us;
}

void PWM_Start(uint8_t aIndex)
{
    // assert(PWMA_QTY > aIndex);

    uint16_t lMasterControl;
    Context* lThis = sContexts + aIndex;

    unsigned int i;

    switch (lThis->mMode)
    {
    case PWM_MODE_CAPTURE_PERIOD:
        for (i = 0; i < INPUT_OUTPUT_QTY; i++)
        {
            lThis->mStates     [i] = STATE_UNKNOWN;
            lThis->mTimeouts_ms[i] = TIMEOUT_ms;
        }

    case PWM_MODE_OUTPUT: break;

    // default: assert(false);
    }

    lMasterControl = COMMON_REGS->mMasterControl;

    COMMON_REGS->mMasterControl = lMasterControl | ((MCTRL_LDOK0 | MCTRL_RUN0) << aIndex);
}

void PWM_Stop(uint8_t aIndex)
{
    // assert(PWMA_QTY > aIndex);

    Context* lThis = sContexts + aIndex;

    unsigned int i;

    for (i = 0; i < INPUT_OUTPUT_QTY; i++)
    {
        lThis->mStates     [i] = STATE_UNKNOWN;
        lThis->mTimeouts_ms[i] = 0;
    }

    COMMON_REGS->mMasterControl &= ~ (MCTRL_RUN0 << aIndex);
}

void PWM_Tick(uint8_t aIndex, unsigned int aPeriod_ms)
{
    // assert(PWMA_QTY > aIndex);

    Context* lThis = sContexts + aIndex;

    if (PWM_MODE_CAPTURE_PERIOD == lThis->mMode)
    {
        unsigned int i;

        for (i = 0; i < INPUT_OUTPUT_QTY; i++)
        {
            if (0 < lThis->mTimeouts_ms[i])
            {
                if (aPeriod_ms < lThis->mTimeouts_ms[i])
                {
                    lThis->mTimeouts_ms[i] -= aPeriod_ms;
                }
                else
                {
                    lThis->mStates     [i] = STATE_ERROR;
                    lThis->mTimeouts_ms[i] = 0;
                }
            }
        }

        Poll(lThis);
    }
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

void Poll(Context* aThis)
{
    // assert(PWMA_QTY > aThis->mIndex);

    volatile ChannelRegs* lR = CHANNEL_REGS + aThis->mIndex;

    uint32_t lCapture[INPUT_OUTPUT_QTY];
    uint16_t lControl[INPUT_OUTPUT_QTY];

    unsigned int i;
    
    lControl[0] = lR->mCaptureControlA;
    lControl[1] = lR->mCaptureControlB;

    // Extract CA0CNT value
    for (i = 0; i < INPUT_OUTPUT_QTY; i++)
    {
        lControl[i] >>= 10;
        lControl[i]  &= 0x7;
    }

    if (0 < lControl[0])
    {
        lCapture[0] = lR->mCaptureValue2;
    }

    if (0 < lControl[1])
    {
        lCapture[1] = lR->mCaptureValue2;
    }

    for (i = 0; i < INPUT_OUTPUT_QTY; i++)
    {
        if (0 < lControl[i])
        {
            switch (aThis->mStates[i])
            {
            case STATE_ERROR:
            case STATE_UNKNOWN:
                aThis->mStates[i] = STATE_WAIT;
                break;

            case STATE_KNOWN:
            case STATE_WAIT:
                aThis->mValues[i] = (lCapture[i] + 0x10000) - aThis->mCaptures[i];
                aThis->mStates[i] = STATE_KNOWN;
                break;

            // default: assert(false);
            }

            aThis->mCaptures   [i] = lCapture[i];
            aThis->mTimeouts_ms[i] = TIMEOUT_ms;
        }
    }    
}

void ReadCapture(Context* aThis)
{
    if (PWM_MODE_CAPTURE_PERIOD == aThis->mMode)
    {
        // assert(PWMA_QTY > aThis->mIndex);

        volatile ChannelRegs* lR = CHANNEL_REGS + aThis->mIndex;

        uint16_t lStatus = lR->mStatus;

        if (0 != (lStatus & 0x0400)) // CFA0
        {
            lR->mStatus |= 0x0400; // CFA0
        }

        if (0 != (lStatus & 0x0100)) // CFB0
        {
            lR->mStatus |= 0x0100; // CFB0
        }

        Poll(aThis);
    }
}
