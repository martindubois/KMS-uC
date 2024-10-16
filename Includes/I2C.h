
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/I2C.h

#pragma once

// Data type
// //////////////////////////////////////////////////////////////////////////

#define I2C_ERROR   (0)
#define I2C_PENDING (1)
#define I2C_SUCCESS (2)

// Functions
// //////////////////////////////////////////////////////////////////////////

extern void I2Cs_Init0();

extern void I2C_Init(uint8_t aIndex);

// Return  false
//         true
extern uint8_t I2C_Idle(uint8_t aIndex);

// Return  I2C_ERROR
//         I2C_PENDING
//         I2C_SUCCESS
extern uint8_t I2C_Status(uint8_t aIndex);

extern void I2C_Read(uint8_t aIndex, uint8_t aDevice, void* aOut, uint8_t aOutSize_byte);

extern void I2C_Write(uint8_t aIndex, uint8_t aDevice, uint8_t aAddr, const void* aIn, uint8_t aInSize_byte);

extern void I2C_Tick(uint8_t aIndex, uint16_t aPeriod_ms);
