
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Watchdog.h

#pragma once

// Functions
// //////////////////////////////////////////////////////////////////////////

extern void Watchdog_Disable();

// aLock  false  Do not protect the watchdog
//        true   Protect the watchdog against any change
extern void Watchdog_Enable(uint8_t aProtect);

extern void Watchdog_Feed();
