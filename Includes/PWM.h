
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/PWM.h

#pragma once

// Constants
// //////////////////////////////////////////////////////////////////////////

#define PWM_ERROR (0xffffffff)

#define PWM_MODE_CAPTURE_PERIOD (0)
#define PWM_MODE_OUTPUT         (1)

// Functions
// //////////////////////////////////////////////////////////////////////////

extern void PWMs_Init();

// Call this function only once for each used PWM channel after the reset of
// the microcontroller.
//
// aIndex  The PWM instance index
// aMode   See PWMA_MODE_...
extern void PWM_Init(uint8_t aIndex, uint8_t aMode);

// aOutput     0  Output A
//             1  Output B
// aDutyCycle  0    =   0 % = always off (stopped)
//             1000 = 100 % = always on
extern void PWM_Set(uint8_t aIndex, uint8_t aOutput, uint16_t aDutyCycle);

// aDutyCycleA  0    =   0 % = always off (stopped)
//              1000 = 100 % = always on
// aDutyCycleB
extern void PWM_Set2(uint8_t aIndex, uint16_t aDutyCycleA, uint16_t aDutyCycleB);

// aInput  0  Input A
//         1  Input B
//
// Return  PWMA_ERROR  No signal received
//         Other       The time between two signal in us
extern uint32_t PWM_Read(uint8_t aIndex, uint8_t aInput);

extern void PWM_Start(uint8_t aIndex);

extern void PWM_Stop(uint8_t aIndex);

// Tick is only needed int PWMA_MODE_CAPTURE_PERIOD
extern void PWM_Tick(uint8_t aIndex, unsigned int aPeriod_ms);
