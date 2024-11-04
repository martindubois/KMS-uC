
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
// Chapter 21 - Computer Operating Properly (COP) Watchdog

// Assumptions
// //////////////////////////////////////////////////////////////////////////

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////

// Code
// //////////////////////////////////////////////////////////////////////////

// ===== C ==================================================================
#include <stdint.h>

// ===== Includes ===========================================================
#include "Watchdog.h"

// Data type
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint16_t mControl;
    uint16_t mTimeout;
    uint16_t mCounter;
    uint16_t mIntVal;
}
COP_Regs;

static volatile COP_Regs* REGS = (COP_Regs*)0xe320;

// Functions
// //////////////////////////////////////////////////////////////////////////

void Watchdog_Disable()
{
    REGS->mControl = 0x0300; // PSS = 1024
}

void Watchdog_Enable(uint8_t aProtect)
{
    uint16_t lCtrl = 0x0302; // PSS = 1024, CEN

    if (aProtect)
    {
        lCtrl |= 0x0001; // CWP
    }

    REGS->mControl = lCtrl;
}

void Watchdog_Feed()
{
    REGS->mCounter = 0x5555;
    REGS->mCounter = 0xaaaa;
}
