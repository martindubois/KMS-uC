
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Modbus_Slave.h

#pragma once

// ===== Includes ===========================================================
#include "GPIO.h"

// Data type
// //////////////////////////////////////////////////////////////////////////

struct Modbus_Slave_Range_s;

// Return  MODBUS_NO_ERROR
//         MODBUS_EXCEPTION_...
typedef uint8_t (*Modbus_Slave_Callback)(struct Modbus_Slave_Range_s* aRange, uint16_t aAddress, uint8_t aCount, uint16_t* aData);

// mContext      Way to pass data to the callbacks
// mAddress      The starting address in the Modbus address space
// mCount        Number of 16 bits registers. Must be at least 1.
// mData         Optional. The address of the data storage. If NULL
//               - mAfterRead must set aData
//               - mAfterWrite or mBeforeWrite must save aData to internal
//                 storage if needed
// mAfterRead    Mandatory. If not needed, set it to
//               Modbus_Slave_Callback_Default.
// mAfterWrite   Mandatory. If not needed, set it to
//               Modbus_Slave_Callback_Default.
// mBeforeWrite  Mandatory. If not needed, set it to
//               Modbus_Slave_Callback_Default. If the range is read only,
//               set it to Modbus_Slave_Callback_Error
typedef struct Modbus_Slave_Range_s
{
    void* mContext;

    uint16_t mAddress;
    uint16_t mCount  ;

    uint16_t* mData;

    Modbus_Slave_Callback mAfterRead  ;
    Modbus_Slave_Callback mAfterWrite ;
    Modbus_Slave_Callback mBeforeWrite;
}
Modbus_Slave_Range;

// Functions
// //////////////////////////////////////////////////////////////////////////

// aUART          The UART index
// aDevice        The Modbus device address
// aRanges        The register ranges
// aRangeQty      The number of ranges
// aOutputEnable  A GPIO to set to 1 when transmiting. Use a GPIO in
//                GPIO_PORT_DUMMY if no such ouput is needed.
extern void Modbus_Slave_Init(uint8_t aUART, uint8_t aDevice, Modbus_Slave_Range* aRanges, uint8_t aRangeQty, GPIO aOutputEnable);

// Return  MODBUS_NO_ERROR
extern uint8_t Modbus_Slave_Callback_Default(struct Modbus_Slave_Range_s* aRange, uint16_t aAddress, uint8_t aCount, uint16_t* aData);

// Return  MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS
extern uint8_t Modbus_Slave_Callback_Error(struct Modbus_Slave_Range_s* aRange, uint16_t aAddress, uint8_t aCount, uint16_t* aData);

extern void Modbus_Slave_Tick(uint16_t aPeriod_ms);

extern void Modbus_Slave_Work();
