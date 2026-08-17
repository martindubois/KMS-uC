
// Product   KMS-uC
// License   http://www.apache.org/licenses/LICENSE-2.0

/// \author    KMS - Martin Dubois, P. Eng.
/// \copyright Copyright &copy; 2024-2026 KMS
/// \file      Includes/Modbus_Slave.h
/// \brief     Functions to process Modbus commands

#pragma once

// ===== Includes ===========================================================
#include "GPIO.h"

// Data type
// //////////////////////////////////////////////////////////////////////////

struct Modbus_Slave_Range_s;

/// \brief Modbus callback
/// \param aRange   The address range
/// \param aAddress Start address
/// \param aCount   Register count
/// \param aData    Data
/// \retval MODBUS_NO_ERROR
/// \retval MODBUS_EXCEPTION_...
typedef uint8_t (*Modbus_Slave_Callback)(struct Modbus_Slave_Range_s* aRange, uint16_t aAddress, uint16_t aCount, uint16_t* aData);

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

/// \brief Modbus address rance
/// \see Modbus_Slave_Callback Modbus_Slave_Init
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

/// \brief Initialize the module
/// \param aUART         The UART index
/// \param aDevice       The Modbus device address
/// \param aRanges       The register ranges
/// \param aRangeQty     The number of ranges
/// \param aOutputEnable This GPIO to set to 1 when transmiting.

// .mBit
// .mDrive
// .mInterrupt_Falling
// .mOutput            : Ignored, must be an output
// .mPort              : See GPIO_PORT_...
// .mPull_Enable
// .mPullUp_Select
// .mPushPull
// .mSlewRate_Slow     : Ignored, must be set
extern void Modbus_Slave_Init(uint8_t aUART, uint8_t aDevice, Modbus_Slave_Range* aRanges, uint8_t aRangeQty, GPIO aOutputEnable);

/// \brief Default callback
/// \param aRange   The address range
/// \param aAddress Start address
/// \param aCount   Register count
/// \param aData    Data
/// \retval MODBUS_NO_ERROR
/// \see Modbus_Slave_Callback
extern uint8_t Modbus_Slave_Callback_Default(struct Modbus_Slave_Range_s* aRange, uint16_t aAddress, uint16_t aCount, uint16_t* aData);

/// \brief Default callback
/// \param aRange   The address range
/// \param aAddress Start address
/// \param aCount   Register count
/// \param aData    Data
/// \retval MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS
/// \see Modbus_Slave_Callback
extern uint8_t Modbus_Slave_Callback_Error(struct Modbus_Slave_Range_s* aRange, uint16_t aAddress, uint16_t aCount, uint16_t* aData);

/// \brief Periodic work
/// \param aPeriod_ms Delay since the last call
extern void Modbus_Slave_Tick(uint16_t aPeriod_ms);

/// \brief Idle work
extern void Modbus_Slave_Work();
