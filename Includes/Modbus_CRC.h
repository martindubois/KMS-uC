
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Modbus_CRC.h

#pragma once

// Data type
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    uint16_t mValue;
}
Modbus_CRC;

// Functions
// //////////////////////////////////////////////////////////////////////////

// aInOut        Buffer containing data and where the CRC is added.
// aInSize_byte  Size of data already in the buffer (excluding CRC)
extern void Modbus_CRC_Compute_Buffer(uint8_t* aInOut, uint8_t aInSize_byte);

// aIn           The received data
// aInSize_byte  Size of data in the buffer (including CRC)
//
// Return  false  Invalid CRC
//         true   Valid CRC
extern uint8_t Modbus_CRC_Verify_Buffer(const uint8_t* aIn, uint8_t aInSize_byte);

extern void Modbus_CRC_Init(Modbus_CRC* aThis);

// aOut  Where to put the CRC value
extern void Modbus_CRC_Get(const Modbus_CRC* aThis, uint8_t* aOut);

// aByte  The new byte
extern void Modbus_CRC_Compute_Byte(Modbus_CRC* aThis, uint8_t aByte);

// aIn  Where the received CRC is
//
// Return  false  Invalid CRC
//         true   Good CRC
extern uint8_t Modbus_CRC_Verify(const Modbus_CRC* aThis, const uint8_t* aIn);
