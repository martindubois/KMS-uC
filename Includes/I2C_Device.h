
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/I2C_Device.h

#pragma once

// Data type
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint8_t mAddress;
    uint8_t mBusIndex;
}
I2C_Device;

// Functions
// //////////////////////////////////////////////////////////////////////////

#define I2C_Device_Idle(T) I2C_Idle((T).mBusIndex)

#define I2C_Device_Status(T) I2C_Status((T).mBusIndex)

#define I2C_Device_Read(T, O, S) I2C_Read((T).mBusIndex, (T).mAddress, (O), (S))

#define I2C_Device_Write(T, A, I, S) I2C_Write((T).mBusIndex, (T).mAddress, (A), (I), (S))

#define I2C_Device_Tick(T, P) I2C_Tick((T).mBusIndex, P)

extern void I2C_Device_Init(I2C_Device* aThis, uint8_t aBusIndex, uint8_t aAddress);
