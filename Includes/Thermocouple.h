
// Product   KMS-uC
// License   http://www.apache.org/licenses/LICENSE-2.0

/// \author    KMS - Martin Dubois, P. Eng.
/// \copyright Copyright &copy; 2024-2026 KMS
/// \file      Includes/Thermocouple.h
/// \brief     Functions to convert thermocouple reading

#pragma once

// Data types
// //////////////////////////////////////////////////////////////////////////

/// \brief Temperature conversion table
typedef struct
{
    int16_t mBegin_C;
    int16_t mEnd_C;
    int16_t mStep_C;

    uint16_t mCount;

    const int16_t* mTable_uV;
}
Thermocouple_Table;

/// \brief Thermocouple type description
typedef struct
{
    const Thermocouple_Table* mTable;
    const Thermocouple_Table* mTable_CJ;
}
Thermocouple_Type;

/// \brief Thermocouple description
typedef struct
{
    int16_t mCal_Gain; // fixed point 4.12
    int16_t mCal_Offset_uV;

    const Thermocouple_Type* mType;
}
Thermocouple;

// Constants
// //////////////////////////////////////////////////////////////////////////

/// \brief Thermocouple type R
extern const Thermocouple_Type Thermocouple_TYPE_R;

// Functions
// //////////////////////////////////////////////////////////////////////////

/// \brief Initialize a Thermocouple instance
/// \param aInit The instance to initialize
/// \param aType Thermocouple_TYPE_...
/// \see Thermocouple_Calibrate Thermocouple_Calibrate2 Thermocouple_TYPE_R Thermocouple_uV_to_C
extern void Thermocouple_Init(Thermocouple* aThis, const Thermocouple_Type* aType);

/// \brief Convert uV value in temperature
/// \param aThis     The Thermocouple instance
/// \param aTempCJ_C Temperature at the cold junction
/// \param aIn_uV    The read value
/// \param aOut_C    The function put the temperature there
/// \retval false No overflow
/// \retval true  Overflow
/// \see Thermocouple_Init
extern uint8_t Thermocouple_uV_to_C(const Thermocouple* aThis, int16_t aTempCJ_C, int32_t aIn_uV, int16_t* aOut_C);

/// \brief Modify the calibration
/// \param aThis   The instance
/// \param aRead_C The indicated temperature
/// \param aReal_C The real temperature
extern void Thermocouple_Calibrate(Thermocouple* aThis, int16_t aRead_C, int16_t aReal_C);

/// \brief Modify the calibration
/// \param aThis   The instance
/// \param aARead_C The indicated temperature
/// \param aAReal_C The real temperature
/// \param aBRead_C The indicated temperature
/// \param aBReal_C The real temperature
extern void Thermocouple_Calibrate2(Thermocouple* aThis, int16_t aARead_C, int16_t aAReal_C, int16_t aBRead_C, int16_t aBReal_C);
