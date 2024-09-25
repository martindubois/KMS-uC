
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/MC56F/ADC12.c

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////
//
// Project configuration
// - Define _MC56F84565_

// Code
// //////////////////////////////////////////////////////////////////////////
//
// - Configure analog input pin using GPIO module

// ===== C ==================================================================
#include <stdint.h>

// ===== Includes ===========================================================
#include "MC56F_SIM.h"

#include "ADC.h"

// Data type
// //////////////////////////////////////////////////////////////////////////

#define CHANNEL_QTY (16)

typedef struct
{
    uint16_t mCtrl1;
    uint16_t mCtrl2;
    uint16_t mZeroCrossingCtrl1;
    uint16_t mZeroCrossingCtrl2;
    uint16_t mChannelList[CHANNEL_QTY / 4];
    uint16_t mSampleDisable;
    uint16_t mStatus;
    uint16_t mReady;
    uint16_t mLowLimitStatus;
    uint16_t mHighLimitStatus;
    uint16_t mZeroCrossingStatus;
    uint16_t mResults   [CHANNEL_QTY];
    uint16_t mLowLimits [CHANNEL_QTY];
    uint16_t mHighLimits[CHANNEL_QTY];
    uint16_t mOffsets   [CHANNEL_QTY];
    uint16_t mPowerCtrl1;
    uint16_t mCalibration;
    uint16_t mGainCtrl1;
    uint16_t mGainCtrl2;
    uint16_t mScanCtrl;
    uint16_t mPowerCtrl2;
    uint16_t mCtrl3;
    uint16_t mScanHaltedInterruptEnable;
}
PortRegs;

static volatile PortRegs* REGISTERS = (PortRegs*)0xe500;

#define CTRL1_STOP0 (0x4000)

#define STAT_EOSI1 (0x1000)
#define STAT_EOSI0 (0x0800)
#define STAT_ZCI   (0x0400)
#define STAT_LLMTI (0x0200)
#define STAT_HLMTI (0x0100)

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static uint8_t GetValue(uint8_t aChannel, uint16_t* aOut);

// Functions
// //////////////////////////////////////////////////////////////////////////

void ADC_Init(const uint8_t* aChannel, uint8_t aChannelQty, uint8_t aInterrupts)
{
    // assert(NULL != aChannel);
    // assert(0 < aChannelQty);
    // assert(CHANNEL_QTY >= aChannelQty);

    uint16_t lCtrl1;
    uint16_t lValue;

    unsigned int i;

    *SIM_PCE2 |= 0x0080; // CYCADC

    lCtrl1 = CTRL1_STOP0 | 0x0002; // SMODE = 2

    if (0 != (aInterrupts & ADC_INTERRUPT_END_OF_SCAN  )) { lCtrl1 |= 0x0800; } // EOSIE0
    if (0 != (aInterrupts & ADC_INTERRUPT_HIGH_LIMIT   )) { lCtrl1 |= 0x0100; } // HLMTIE
    if (0 != (aInterrupts & ADC_INTERRUPT_LOW_LIMIT    )) { lCtrl1 |= 0x0200; } // LLMTIE
    if (0 != (aInterrupts & ADC_INTERRUPT_ZERO_CROSSING)) { lCtrl1 |= 0x0400; } // ZCIE

    REGISTERS->mCtrl1             = lCtrl1;
    REGISTERS->mCtrl2             = 0x003f; // DIV0 = 64 (80 MHz / 64 = 1.25 MHz, T = 0.8 us)
    REGISTERS->mCtrl3             = 0x0007; // SCNT0 = 7
    REGISTERS->mCalibration       = 0;
    // REGISTERS->mGainCtrl1         = 0; // Default
    // REGISTERS->mGainCtrl2         = 0; // Default
    // REGISTERS->mPowerCtrl2        = 0; // Default
    REGISTERS->mSampleDisable     = 0xffff << aChannelQty;
    // REGISTERS->mZeroCrossingCtrl1 = 0; // Default
    // REGISTERS->mZeroCrossingCtrl2 = 0; // Default

    lValue = 0;

    for (i = 0; i < CHANNEL_QTY; i++)
    {
        uint8_t lMod = i % 4;

        // REGISTERS->mHighLimits[i] = 0x7ff8; // Default
        // REGISTERS->mLowLimits [i] = 0x0000; // Default

        if (aChannelQty > i)
        {
            if (0 == (aChannel[i] & ADC_SIGNED))
            {
                REGISTERS->mOffsets[i] = 0x4000;
            }

            lValue |= (aChannel[i] & 0x0f) << (4 * lMod);
        }

        if (3 == lMod)
        {
            REGISTERS->mChannelList[i / 4] = lValue;
            lValue = 0;
        }
    }

    REGISTERS->mPowerCtrl1        = 0x001a2; // PUDELAY = 01 1010 = 26 clock, PD0

    // Clear status
    REGISTERS->mLowLimitStatus     = 0xffff;
    REGISTERS->mHighLimitStatus    = 0xffff;
    REGISTERS->mStatus             = STAT_EOSI0 | STAT_EOSI1 | STAT_HLMTI | STAT_LLMTI | STAT_ZCI;
    REGISTERS->mZeroCrossingStatus = 0xffff;

    lCtrl1 &= ~ CTRL1_STOP0;

    REGISTERS->mCtrl1 = lCtrl1;

    while (REGISTERS->mPowerCtrl1 & 0x0400) {} // PSTS0

    lCtrl1 |= 0x2000; // START0

    REGISTERS->mCtrl1 = lCtrl1;
}

uint8_t ADC_GetValue_Signed(uint8_t aChannel, int16_t* aOut)
{
    return GetValue(aChannel, (uint16_t*)aOut); // reinterpret_cast
}

uint8_t ADC_GetValue_Unsigned(uint8_t aChannel, uint16_t* aOut)
{
    return GetValue(aChannel, aOut);
}

void ADC_SetLimits(uint8_t aChannel, uint16_t aLow, uint16_t aHigh)
{
    // assert(CHANNEL_QTY > aChannel);
    // assert(aLow < aHigh);

    REGISTERS->mHighLimits[aChannel] = aHigh;
    REGISTERS->mLowLimits [aChannel] = aLow;
}

void ADC_AcknowledgeInterrupt()
{
    uint16_t lHLS;
    uint16_t lLLS;
    uint16_t lS;
    uint16_t lZCS;

    lHLS = REGISTERS->mHighLimitStatus;
    lLLS = REGISTERS->mLowLimitStatus;
    lS   = REGISTERS->mStatus;
    lZCS = REGISTERS->mZeroCrossingStatus;

    lS &= STAT_EOSI1 | STAT_EOSI0 | STAT_ZCI | STAT_LLMTI | STAT_HLMTI;

    REGISTERS->mHighLimitStatus    = lHLS;
    REGISTERS->mLowLimitStatus     = lLLS;
    REGISTERS->mStatus             = lS;
    REGISTERS->mZeroCrossingStatus = lZCS;
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

uint8_t GetValue(uint8_t aChannel, uint16_t* aOut)
{
    // assert(CHANNEL_QTY > aChannel);
    // assert(NULL != aOut);

    uint16_t lB = 1 << aChannel;
    uint16_t lHLS;
    uint16_t lLLS;
    uint16_t lR;
    uint8_t  lResult = 0;
    uint16_t lZCS;

    lHLS = REGISTERS->mHighLimitStatus;
    lLLS = REGISTERS->mLowLimitStatus;
    lR   = REGISTERS->mReady;
    lZCS = REGISTERS->mZeroCrossingStatus;

    if (0 != (lB & lHLS))
    {
        lResult |= ADC_HIGH_LIMIT;
        REGISTERS->mHighLimitStatus = lB;
    }

    if (0 != (lB & lLLS))
    {
        lResult |= ADC_LOW_LIMIT;
        REGISTERS->mLowLimitStatus = lB;
    }

    if (0 == (lB & lR))
    {
        lResult |= ADC_NOT_READY;
    }

    if (0 != (lB & lZCS))
    {
        lResult |= ADC_ZERO_CROSSING;
        REGISTERS->mZeroCrossingStatus = lB;
    }

    *aOut = REGISTERS->mResults[aChannel];

    return lResult;
}
