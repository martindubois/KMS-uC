
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/MC56F/GPIO.c

// References
// //////////////////////////////////////////////////////////////////////////
//
// MC56F8458x Reference Manual with Addendum
// https://www.nxp.com/docs/en/reference-manual/MC56F8458XRM.pdf
// Chapter 38 - General-Purpose Input/Output (GPIO)

// Assumptions
// //////////////////////////////////////////////////////////////////////////

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////
//
// Project configuration
// - Define _MC56F84565_
//
// Processor expert configuration
// - If interrupt are used, add an InterruptVector for it.

// Code
// //////////////////////////////////////////////////////////////////////////

// ===== C ==================================================================
#include <stdint.h>
#include <stdlib.h>

// ===== Includes ===========================================================
#include "MC56F_SIM.h"

#include "GPIO.h"

// Data types
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint16_t mPullUp_Enable;
    uint16_t mData;
    uint16_t mOutput;
    uint16_t mPeripheral_Enable;
    uint16_t mInterrupt_Assert;
    uint16_t mInterrupt_Enable;
    uint16_t mInterrupt_Falling;
    uint16_t mInterrupt_Pending;
    uint16_t mInterrupt_EdgeDetected;
    uint16_t mPushPull;
    uint16_t mData_Raw;
    uint16_t mDrive;
    uint16_t mPullUp_Select;
    uint16_t mSlewRate_Slow;
    uint16_t mReserved0[2];
}
PortRegs;

// Constants
// //////////////////////////////////////////////////////////////////////////

#define BIT_PER_PORT (16)

static volatile PortRegs* PORT_REGS = (PortRegs*)0x0000E200;

static const uint16_t SIM_PCE0_BITS[GPIO_PORT_DUMMY] =
{
    0x0040,
    0x0020,
    0x0010,
    0x0008,
    0x0004,
    0x0002,
    0x0001,
};

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static void Init(GPIO aD);

// Functions
// //////////////////////////////////////////////////////////////////////////

void GPIO_Init(GPIO aDesc)
{
    // assert(BIT_PER_PORT > aDesc.mBit);

    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        uint16_t           lB;
        uint16_t           lM;
        volatile PortRegs* lR;

        lB = 1 << aDesc.mBit;
        lM = ~ lB;
        lR = PORT_REGS + aDesc.mPort;

        Init(aDesc);

        lR->mPeripheral_Enable &= lM;

        if (aDesc.mOutput) { lR->mOutput |= lB; } else { lR->mOutput &= lM; }
    }
}

void GPIO_InitFunction(GPIO aDesc, uint16_t aFunction)
{
    // assert(BIT_PER_PORT > aDesc.mBit);

    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        uint16_t           lB;
        volatile uint16_t* lGPS = NULL;
        uint16_t           lM;
        volatile PortRegs* lR;
        unsigned int       lS2;

        lB  = 1 << aDesc.mBit;
        lM  = ~ lB;
        lR  = PORT_REGS + aDesc.mPort;
        lS2 = 2 * aDesc.mBit;

        if (8 <= aDesc.mBit)
        {
            lS2 -= BIT_PER_PORT;
        }

        switch (aDesc.mPort)
        {
        case GPIO_PORT_A: lGPS = (7 >= aDesc.mBit) ? SIM_GPSAL : NULL     ; break;
        case GPIO_PORT_B: lGPS = (7 >= aDesc.mBit) ? NULL      : SIM_GPSBH; break;
        case GPIO_PORT_C: lGPS = (7 >= aDesc.mBit) ? SIM_GPSCL : SIM_GPSCH; break;
        case GPIO_PORT_D: lGPS = (7 >= aDesc.mBit) ? SIM_GPSDL : NULL     ; break;
        }

        if (NULL != lGPS)
        {
            *lGPS &= ~ (0x3 << lS2);
            *lGPS |= aFunction << lS2;
        }

        Init(aDesc);

        lR->mOutput            &= lM;
        lR->mPeripheral_Enable |= lB;
    }
}

void GPIO_GetRegisterAndMask(GPIO aDesc, volatile uint16_t** aReg, uint16_t* aMask)
{
    // assert(GPIO_PORT_DUMMY > aDesc.mPort);

    uint16_t           lB;
    volatile PortRegs* lR;

    lB = 1 << aDesc.mBit;
    lR = PORT_REGS + aDesc.mPort;

    *aMask = lB;
    *aReg  = &lR->mData;
}

uint8_t GPIO_Input(GPIO aDesc)
{
    // assert(BIT_PER_PORT > aDesc.mBit);

    uint8_t lResult = 0;

    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        uint16_t           lB;
        volatile PortRegs* lR;

        lB = 1 << aDesc.mBit;
        lR = PORT_REGS + aDesc.mPort;

        lResult = (0 != (lR->mData & lB));
    }

    return lResult;
}

void GPIO_Interrupt_Acknowledge(GPIO aDesc)
{
    // assert(BIT_PER_PORT > aDesc.mBit);

    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        uint16_t           lB;
        volatile PortRegs* lR;

        lB = 1 << aDesc.mBit;
        lR = PORT_REGS + aDesc.mPort;

        lR->mInterrupt_EdgeDetected = lB;
    }
}

void GPIO_Interrupt_Disable(GPIO aDesc)
{
    // assert(BIT_PER_PORT > aDesc.mBit);

    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        uint16_t           lB;
        uint16_t           lM;
        volatile PortRegs* lR;

        lB = 1 << aDesc.mBit;
        lM = ~ lB;
        lR = PORT_REGS + aDesc.mPort;

        lR->mInterrupt_Enable &= lM;
    }
}

void GPIO_Interrupt_Enable(GPIO aDesc)
{
    // assert(BIT_PER_PORT > aDesc.mBit);

    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        uint16_t           lB;
        volatile PortRegs* lR;

        lB = 1 << aDesc.mBit;
        lR = PORT_REGS + aDesc.mPort;

        lR->mInterrupt_Enable |= lB;
    }
}

void GPIO_Output(GPIO aDesc, uint8_t aVal)
{
    // assert(BIT_PER_PORT > aDesc.mBit);

    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        uint16_t           lB;
        volatile PortRegs* lR;

        lB = 1 << aDesc.mBit;
        lR = PORT_REGS + aDesc.mPort;

        if (aVal)
        {
            lR->mData |=   lB;
        }
        else
        {
            lR->mData &= ~ lB;
        }
    }
}

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

void Init(GPIO aDesc)
{
    // assert(BIT_PER_PORT > aDesc.mBit);
    // assert(GPIO_PORT_DUMMY > aDesc.mPort);

    uint16_t           lB;
    uint16_t           lM;
    volatile PortRegs* lR;

    lB = 1 << aDesc.mBit;
    lM = ~ lB;
    lR = PORT_REGS + aDesc.mPort;

    *SIM_PCE0 |= SIM_PCE0_BITS[aDesc.mPort];

    if (aDesc.mDrive            ) { lR->mDrive             |= lB; } else { lR->mDrive             &= lM; }
    if (aDesc.mInterrupt_Falling) { lR->mInterrupt_Falling |= lB; } else { lR->mInterrupt_Falling &= lM; }
    if (aDesc.mPull_Enable      ) { lR->mPullUp_Enable     |= lB; } else { lR->mPullUp_Enable     &= lM; }
    if (aDesc.mPullUp_Select    ) { lR->mPullUp_Select     |= lB; } else { lR->mPullUp_Select     &= lM; }
    if (aDesc.mPushPull         ) { lR->mPushPull          |= lB; } else { lR->mPushPull          &= lM; }
    if (aDesc.mSlewRate_Slow    ) { lR->mSlewRate_Slow     |= lB; } else { lR->mSlewRate_Slow     &= lM; }
}
