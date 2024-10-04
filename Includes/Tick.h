
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Tick.h

#pragma once

// Functions
// //////////////////////////////////////////////////////////////////////////

extern void Tick_Init(uint32_t aClock_Hz);

// Return  0      No tick
//         Other  Tick period in ms
extern uint16_t Tick_Work();
