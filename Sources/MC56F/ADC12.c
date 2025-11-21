
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/MC56F/ADC12.c

// References
// //////////////////////////////////////////////////////////////////////////
//
// MC56F8458x Reference Manual with Addendum
// https://www.nxp.com/docs/en/reference-manual/MC56F8458XRM.pdf
// Chapter 25 - 12-bit Cyclic Analog-to-Digital Converter (ADC12)

// Assumptions
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
// - If ADC_INTERRUPT_END_OF_SCAN is passed to ADC_Init
//     - Add an InterruptVector for INT_ADC12_CC0
// - If ADC_INTERRUPT_HIGH_LIMIT, ADC_INTERRUPT_LOW_LIMIT or
//   ADC_INTERRUPT_ZERO_CROSSING are passed to ADC_Init
//     - Add and InterruptVector for INT_ADC12_ERR

// Code
// //////////////////////////////////////////////////////////////////////////
//
// - Configure analog input pin using GPIO_InitFuncton

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
ADC12_Regs;

static volatile ADC12_Regs* REGS = (ADC12_Regs*)0xe500;

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

    REGS->mCtrl1             = lCtrl1;
    REGS->mCtrl2             = 0x003f; // DIV0 = 64 (80 MHz / 64 = 1.25 MHz, T = 0.8 us)
    REGS->mCtrl3             = 0x0007; // SCNT0 = 7
    REGS->mCalibration       = 0;
    // REGS->mGainCtrl1         = 0; // Default
    // REGS->mGainCtrl2         = 0; // Default
    // REGS->mPowerCtrl2        = 0; // Default
    REGS->mSampleDisable     = 0xffff << aChannelQty;
    // REGS->mZeroCrossingCtrl1 = 0; // Default
    // REGS->mZeroCrossingCtrl2 = 0; // Default

    lValue = 0;

    for (i = 0; i < CHANNEL_QTY; i++)
    {
        uint8_t lMod = i % 4;

        // REGS->mHighLimits[i] = 0x7ff8; // Default
        // REGS->mLowLimits [i] = 0x0000; // Default

        if (aChannelQty > i)
        {
            if (0 != (aChannel[i] & ADC_SIGNED))
            {
                REGS->mOffsets[i] = 0x4000;
            }

            lValue |= (aChannel[i] & 0x0f) << (4 * lMod);
        }

        if (3 == lMod)
        {
            REGS->mChannelList[i / 4] = lValue;
            lValue = 0;
        }
    }

    REGS->mPowerCtrl1        = 0x001a0; // PUDELAY = 01 1010 = 26 clock

    // Clear status
    REGS->mLowLimitStatus     = 0xffff;
    REGS->mHighLimitStatus    = 0xffff;
    REGS->mStatus             = STAT_EOSI0 | STAT_EOSI1 | STAT_HLMTI | STAT_LLMTI | STAT_ZCI;
    REGS->mZeroCrossingStatus = 0xffff;

    lCtrl1 &= ~ CTRL1_STOP0;

    REGS->mCtrl1 = lCtrl1;

    while (REGS->mPowerCtrl1 & 0x0c00) {} // PTS1, PSTS0

    lCtrl1 |= 0x2000; // START0

    REGS->mCtrl1 = lCtrl1;
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

    REGS->mHighLimits[aChannel] = aHigh;
    REGS->mLowLimits [aChannel] = aLow;
}

void ADC_AcknowledgeInterrupt()
{
    uint16_t lHLS;
    uint16_t lLLS;
    uint16_t lS;
    uint16_t lZCS;

    lHLS = REGS->mHighLimitStatus;
    lLLS = REGS->mLowLimitStatus;
    lS   = REGS->mStatus;
    lZCS = REGS->mZeroCrossingStatus;

    lS &= STAT_EOSI1 | STAT_EOSI0 | STAT_ZCI | STAT_LLMTI | STAT_HLMTI;

    REGS->mHighLimitStatus    = lHLS;
    REGS->mLowLimitStatus     = lLLS;
    REGS->mStatus             = lS;
    REGS->mZeroCrossingStatus = lZCS;
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

    lHLS = REGS->mHighLimitStatus;
    lLLS = REGS->mLowLimitStatus;
    lR   = REGS->mReady;
    lZCS = REGS->mZeroCrossingStatus;

    if (0 != (lB & lHLS))
    {
        lResult |= ADC_HIGH_LIMIT;
        REGS->mHighLimitStatus = lB;
    }

    if (0 != (lB & lLLS))
    {
        lResult |= ADC_LOW_LIMIT;
        REGS->mLowLimitStatus = lB;
    }

    if (0 == (lB & lR))
    {
        lResult |= ADC_NOT_READY;
    }

    if (0 != (lB & lZCS))
    {
        lResult |= ADC_ZERO_CROSSING;
        REGS->mZeroCrossingStatus = lB;
    }

    *aOut = REGS->mResults[aChannel];

    return lResult;
}
