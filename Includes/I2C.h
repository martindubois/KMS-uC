
// Product   KMS-uC
// License   http://www.apache.org/licenses/LICENSE-2.0

/// \author    KMS - Martin Dubois, P. Eng.
/// \copyright Copyright &copy; 2024-2026 KMS
/// \file      Includes/I2C.h
/// \brief     Functions to control I2C ports

#pragma once

// Data type
// //////////////////////////////////////////////////////////////////////////

/// \brief I2C speed
typedef struct
{
    uint16_t mSpeed_KHz; ///< Approximate speed in KHz
    uint16_t mInternal;  ///< Value written to the frequency divider register
}
I2C_Speed;

/// \brief Status - The last operation failed
/// \see I2C_Status
#define I2C_ERROR   (0)

/// \brief Status - An operation is in progress
/// \see I2C_Status
#define I2C_PENDING (1)

/// \brief Status - The last operation succeeded
/// \see I2C_Status
#define I2C_SUCCESS (2)

/// \brief Lowest speed index
/// \see I2C_SPEEDS
#define I2C_SPEED_MIN (0)

/// \brief Highest speed index
/// \see I2C_SPEEDS
#define I2C_SPEED_MAX (7)

/// \brief Number of entries in I2C_SPEEDS
#define I2C_SPEED_QTY (8)

// Constants
// //////////////////////////////////////////////////////////////////////////

/// \brief Supported speeds, sorted from the slowest to the fastest
/// \see I2C_Init
extern const I2C_Speed I2C_SPEEDS[I2C_SPEED_QTY];

/// \brief Default speed index
/// \see I2C_Init
/// \see I2C_SPEEDS
extern const uint8_t I2C_SPEED_DEFAULT;

// Functions
// //////////////////////////////////////////////////////////////////////////

/// \brief Initialize the I2C module
///
/// Call this function once, before any other I2C function.
extern void I2Cs_Init0();

/// \brief Initialize an I2C port
/// \param aIndex      Index of the I2C port
/// \param aSpeedIndex Index in I2C_SPEEDS, from I2C_SPEED_MIN to
///                    I2C_SPEED_MAX, or I2C_SPEED_DEFAULT
/// \see I2C_SPEEDS
extern void I2C_Init(uint8_t aIndex, uint8_t aSpeedIndex);

/// \brief Is the I2C port idle?
/// \param aIndex Index of the I2C port
/// \retval false
/// \retval true
extern uint8_t I2C_Idle(uint8_t aIndex);

/// \brief Return the operation status
/// \param aIndex Index of the I2C port
/// \retval I2C_ERROR
/// \retval I2C_PENDING
/// \retval I2C_SUCCESS
extern uint8_t I2C_Status(uint8_t aIndex);

/// \brief Read from an I2C device
/// \param aIndex        Index of the I2C port
/// \param aDevice       I2C device address
/// \param aOut          The function put the data there
/// \param aOutSize_byte Size to read
/// \see I2C_Status
extern void I2C_Read(uint8_t aIndex, uint8_t aDevice, void* aOut, uint8_t aOutSize_byte);

/// \brief Write to an I2C device
/// \param aIndex       Index of the I2C port
/// \param aDevice      I2C device address
/// \param aAddr        Address (register) inside the device
/// \param aIn          Data
/// \param aInSize_byte Data size
/// \see I2C_Status
extern void I2C_Write(uint8_t aIndex, uint8_t aDevice, uint8_t aAddr, const void* aIn, uint8_t aInSize_byte);

/// \brief Execute periodic work
/// \param aIndex     Index of the I2C port
/// \param aPeriod_ms Delay since the last call
extern void I2C_Tick(uint8_t aIndex, uint16_t aPeriod_ms);
