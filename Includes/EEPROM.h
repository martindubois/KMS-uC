
// Product   KMS-uC
// License   http://www.apache.org/licenses/LICENSE-2.0

/// \author    KMS - Martin Dubois, P. Eng.
/// \copyright Copyright &copy; 2024-2026 KMS
/// \file      Includes/EEPROM.h
/// \brief     Functions to control EEPROM

#pragma once

// ===== Includes ===========================================================
#include "GPIO.h"
#include "I2C_Device.h"

// Data type
// //////////////////////////////////////////////////////////////////////////

/// \brief EEPROM
typedef struct
{
    I2C_Device mDevice;

    uint8_t* mDataPtr;

    GPIO     mWriteProtect;

    uint8_t  mAddress;
    uint8_t  mState;

    uint16_t mDataSize_byte;
    uint16_t mReadSize_byte;
    uint16_t mTimeout_ms;

    uint8_t  mBuffer[16];
}
EEPROM;

#define EEPROM_ERROR   (0)
#define EEPROM_PENDING (1)
#define EEPROM_SUCCESS (2)

// Functions
// //////////////////////////////////////////////////////////////////////////

/// \brief Initialize the write protect IO
/// \param aWriteProtect The Write protect IO
///
/// This function use GPIO_Init to initialise the IO.

// .mBit
// .mDrive
// .mInterrupt_Falling
// .mOutput            : Ignored, must be an output
// .mPort              : See GPIO_PORT_...
// .mPull_Enable
// .mPullUp_Select
// .mPushPull
// .mSlewRate_Slow     : Ignored, must be set
void EEPROM_InitWriteProtect(GPIO aWriteProtect);

/// \brief Initialize a EEPROM instance
/// \param aThis           The instance
/// \param aBusIndex       Index of the I2C port
/// \param aDeviceAddress  I2C device address
/// \param aWriteProtect   The write protect pin.
/// \see EEPROM_InitWriteProtect
extern void EEPROM_Init(EEPROM* aThis, uint8_t aBusIndex, uint8_t aDeviceAddress, GPIO aWriteProtect);

/// \brief Erase the EEPROM
/// \param aThis      The instance
/// \param aAddress   Start address
/// \param aSize_byte Size to erase
/// \see EEPROM_Status
extern void EEPROM_Erase(EEPROM* aThis, uint8_t aAddress, uint16_t aSize_byte);

/// \brief Erase the EEPROM and verify
/// \param aThis      The instance
/// \param aAddress   Start address
/// \param aSize_byte Size to erase
/// \see EEPROM_Status
extern void EEPROM_Erase_Verify(EEPROM* aThis, uint8_t aAddress, uint16_t aSize_byte);

/// \brief Is the EEPROM idle?
/// \param aThis      The instance
/// \retval false
/// \retval true
extern uint8_t EEPROM_Idle(EEPROM* aThis);

/// \brief Read from the EEPROM
/// \param aThis         The instance
/// \param aAddress      Start address
/// \param aOut          The function put the data there
/// \param aOutSize_byte Size to read
/// \see EEPROM_Status
extern void EEPROM_Read(EEPROM* aThis, uint8_t aAddress, void* aOut, uint16_t aOutSize_byte);

/// \brief Return the operation status
/// \param aThis The instance
/// \retval EEPROM_ERROR
/// \retval EEPROM_PENDING
/// \retval EEPROM_SUCCESS
extern uint8_t EEPROM_Status(EEPROM* aThis);

/// \brief Execute periodic work
/// \param aThis      The instance
/// \param aPeriod_ms Delay since the last call
extern void EEPROM_Tick(EEPROM* aThis, uint16_t aPeriod_ms);

/// \brief Verify
/// \param aThis        The instance
/// \param aAddress     Start address
/// \param aIn          Expected data
/// \param aInSize_byte Expected data size
/// \see EEPROM_Status
extern void EEPROM_Verify(EEPROM* aThis, uint8_t aAddress, const void* aIn, uint16_t aInSize_byte);

/// \brief Idle work
/// \param aThis The instance
extern void EEPROM_Work(EEPROM* aThis);

/// \brief Write to EEPROM
/// \param aThis        The instance
/// \param aAddress     Start address
/// \param aIn          Data
/// \param aInSize_byte Data size
/// \see EEPROM_Status
extern void EEPROM_Write(EEPROM* aThis, uint8_t aAddress, const void* aIn, uint16_t aInSize_byte);

/// \brief Write to EEPROM and verify
/// \param aThis        The instance
/// \param aAddress     Start address
/// \param aIn          Data
/// \param aInSize_byte Data size
/// \see EEPROM_Status
extern void EEPROM_Write_Verify(EEPROM* aThis, uint16_t aAddress, const void* aIn, uint16_t aInSize_byte);
