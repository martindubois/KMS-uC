
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/MC56F_SIM.h

#pragma once

// Constants
// //////////////////////////////////////////////////////////////////////////

#ifdef _MC56F8006_

    static volatile uint16_t* SIM_PCE = (uint16_t*)0x0000f246;

#endif

#ifdef _MC56F84565_

    // Reference
    // https://www.nxp.com/docs/en/reference-manual/MC56F8458XRM.pdf
    // Page 173

    static volatile uint16_t* SIM_PCE0  = (uint16_t*)0x0000e40c;
    static volatile uint16_t* SIM_PCE1  = (uint16_t*)0x0000e40d;
    static volatile uint16_t* SIM_PCE2  = (uint16_t*)0x0000e40e;
    static volatile uint16_t* SIM_PCE3  = (uint16_t*)0x0000e40f;

    static volatile uint16_t* SIM_GPSAL = (uint16_t*)0x0000e417;
    static volatile uint16_t* SIM_GPSBH = (uint16_t*)0x0000e418;
    static volatile uint16_t* SIM_GPSCL = (uint16_t*)0x0000e419;
    static volatile uint16_t* SIM_GPSCH = (uint16_t*)0x0000e41a;
    static volatile uint16_t* SIM_GPSDL = (uint16_t*)0x0000e41b;

#endif
