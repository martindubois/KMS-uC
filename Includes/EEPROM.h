
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/EEPROM.h

#pragma once

// ===== Includes ===========================================================
#include "GPIO.h"
#include "I2C_Device.h"

// Data type
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    I2C_Device mDevice;

    uint8_t* mDataPtr;

    GPIO     mWriteProtect;

    uint16_t mAddress;
    uint16_t mDataSize_byte;
    uint16_t mReadSize_byte;
    uint16_t mTimeout_ms;

    uint8_t  mBuffer[16];
    uint8_t  mState;
}
EEPROM;

#define EEPROM_ERROR   (0)
#define EEPROM_PENDING (1)
#define EEPROM_SUCCESS (2)

// Functions
// //////////////////////////////////////////////////////////////////////////

// aBusIndex       Index of the I2C port
// aDeviceAddress  I2C device address
// aWriteProtect   The write protect pin.
//                     .mBit
//                     .mDrive
//                     .mInterrupt_Falling
//                     .mOutput            : Ignored, must be an output
//                     .mPort              : See GPIO_PORT_...
//                     .mPull_Enable
//                     .mPullUp_Select
//                     .mPushPull
//                     .mSlewRate_Slow     : Ignored, must be set
extern void EEPROM_Init(EEPROM* aThis, uint8_t aBusIndex, uint8_t aDeviceAddress, GPIO aWriteProtect);

extern void EEPROM_Erase(EEPROM* aThis, uint16_t aAddress, uint16_t aSize_byte);

extern void EEPROM_Erase_Verify(EEPROM* aThis, uint16_t aAddress, uint16_t aSize_byte);

// Return  false
//         true
extern uint8_t EEPROM_Idle(EEPROM* aThis);

extern void EEPROM_Read(EEPROM* aThis, uint16_t aAddress, void* aOut, uint16_t aOutSize_byte);

// Return  EEPROM_ERROR
//         EEPROM_PENDING
//         EEPROM_SUCCESS
extern uint8_t EEPROM_Status(EEPROM* aThis);

extern void EEPROM_Tick(EEPROM* aThis, uint16_t aPeriod_ms);

extern void EEPROM_Verify(EEPROM* aThis, uint16_t aAddress, const void* aIn, uint16_t aInSize_byte);

extern void EEPROM_Work(EEPROM* aThis);

extern void EEPROM_Write(EEPROM* aThis, uint16_t aAddress, const void* aIn, uint16_t aInSize_byte);

extern void EEPROM_Write_Verify(EEPROM* aThis, uint16_t aAddress, const void* aIn, uint16_t aInSize_byte);
