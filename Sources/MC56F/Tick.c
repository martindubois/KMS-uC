
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/MC56F/Tick.c

// ===== C ==================================================================
#include <stdint.h>

// ===== Includes ===========================================================
#include "MC56F_SIM.h"

#include "Tick.h"

// Data types
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint16_t mControl;
    uint16_t mModulo;
    uint16_t mCounter;
}
PIT_Regs;

#define PIT_CTRL_CNT_EN (0x0001)
#define PIT_CTRL_PRF    (0x0004)

// Constants
// //////////////////////////////////////////////////////////////////////////

#define PERIOD_ms (10)

static volatile PIT_Regs* PIT1_REGS = (PIT_Regs*)0x0000E110;

#define SIM_PCE2_PIT1 (0x0004)

// Functions
// //////////////////////////////////////////////////////////////////////////

void Tick_Init(uint32_t aClock_Hz)
{
    // assert(0 < aClock_Hz);

    uint32_t lCount     = aClock_Hz / 100;
    uint8_t  lPrescaler = 0;

    while (0xffff < lCount)
    {
        lCount /= 2;
        lPrescaler++;
    }

    // assert(0xf >= lPrescaler);

    *SIM_PCE2 |= SIM_PCE2_PIT1;

    PIT1_REGS->mControl = lPrescaler << 3;
    PIT1_REGS->mModulo  = lCount;

    PIT1_REGS->mControl = (lPrescaler << 3) | PIT_CTRL_CNT_EN;
}

uint16_t Tick_Work()
{
    uint16_t lControl = PIT1_REGS->mControl;
    uint16_t lResult_ms = 0;

    if (0 != (lControl & PIT_CTRL_PRF))
    {
        lResult_ms = PERIOD_ms;

        lControl &= ~ PIT_CTRL_PRF;

        PIT1_REGS->mControl = lControl;
    }

    return lResult_ms;
}
