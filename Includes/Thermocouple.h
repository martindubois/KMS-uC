
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Thermocouple.h

#pragma once

// Data types
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
    int16_t mBegin_C;
    int16_t mEnd_C;
    int16_t mStep_C;

    uint16_t mCount;

    const int16_t* mTable_uV;
}
Thermocouple_Table;

typedef struct
{
    const Thermocouple_Table* mTable;
    const Thermocouple_Table* mTable_CJ;
}
Thermocouple_Type;

typedef struct
{
    int16_t mCal_Offset_uV;

    const Thermocouple_Type* mType;
}
Thermocouple;

// Constants
// //////////////////////////////////////////////////////////////////////////

extern const Thermocouple_Type THERMOCOUPLE_TYPE_R;

// Functions
// //////////////////////////////////////////////////////////////////////////

// aType  THERMOCOUPLE_TYPE_R
extern void Thermocouple_Init(Thermocouple* aThis, const Thermocouple_Type* aType);

// Return  false
//         true
extern uint8_t Thermocouple_uV_to_C(const Thermocouple* aThis, int16_t aTempCJ_C, int32_t aIn_uV, int16_t* aOut_C);

extern void Thermocouple_Calibrate(Thermocouple* aThis, int16_t aRead_C, int16_t aReal_C);
