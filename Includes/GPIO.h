
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/GPIO.h

#pragma once

// Data types
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint16_t mBit               : 4;
    uint16_t mDrive             : 1;
    uint16_t mFunction          : 2;
    uint16_t mInterrupt_Falling : 1;

    uint16_t mOutput        : 1;
    uint16_t mPort          : 3;
    uint16_t mPullUp_Enable : 1;
    uint16_t mPullUp_Select : 1;
    uint16_t mPushPull      : 1;
    uint16_t mSlewRate_Slow : 1;
}
GPIO;

#define GPIO_PORT_A (0)
#define GPIO_PORT_B (1)
#define GPIO_PORT_C (2)
#define GPIO_PORT_D (3)
#define GPIO_PORT_E (4)
#define GPIO_PORT_F (5)
#define GPIO_PORT_G (6)

#define GPIO_PORT_DUMMY (7)

// Functions
// //////////////////////////////////////////////////////////////////////////

extern void GPIO_Init(GPIO aDesc);

extern void GPIO_InitFunction(GPIO aDesc);

// Retrieve the register address and mask, normaly to be passed to
// Debounced_Init.
//
// aDesc  GPIO Descriptor
// aReg   The function puts the register address there
// aMask  The function puts the mask there
extern void GPIO_GetRegisterAndMask(GPIO aDesc, volatile uint16_t** aReg, uint16_t* aMask);

// Return  false
//         true
extern uint8_t GPIO_Input(GPIO aDesc);

extern void GPIO_Interrupt_Acknowledge(GPIO aDesc);

extern void GPIO_Interrupt_Disable(GPIO aDesc);

extern void GPIO_Interrupt_Enable(GPIO aDesc);

// aDesc   GPIO Descriptor
// aValue  false
//         true
extern void GPIO_Output(GPIO aDesc, uint8_t aValue);

// Return  false
//         true
extern uint8_t GPIO_Output_Get(GPIO aDesc);
