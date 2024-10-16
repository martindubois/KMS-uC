
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/Thermocouple.c

// References
// //////////////////////////////////////////////////////////////////////////
//
// Electronic - Thermocouple
// https://www.kms-quebec.com/Cards/0013_en.pdf

// Assumptions
// //////////////////////////////////////////////////////////////////////////

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////

// Code
// //////////////////////////////////////////////////////////////////////////

// ===== C ==================================================================
#include <stdint.h>

// ===== Includes ===========================================================
#include "Thermocouple.h"

// Constants
// //////////////////////////////////////////////////////////////////////////

static const int16_t R_VALUES[] =
{
        0,    88,   183,   284,   390,   501,   618,   738, //    0 -  112 C
      863, 	 992,  1124,  1260,  1398,  1540,  1684,  1831, //  128 -  240 C
     1980,	2131,  2284,  2440,  2597,  2756,  2916,  3079, //  256 -  368 C
     3242,  3408,  3574,  3742,  3912,  4083,  4255,  4428, //  384 -  496 C
     4602,  4778,  4955,  5133,  5312,  5493,  5674,  5857, //  512 -  624 C
     6041,  6227,  6413,  6601,  6790,  6980,  7172,  7364, //  640 -  752 C
     7558,	7753,  7950,  8147,  8346,  8546,  8748,  8950, //  768 -  880 C
     9154,  9359,  9565,  9772,  9980, 10190, 10400, 10612, //  896 - 1008 C
    10825, 11039, 11253, 11469, 11686, 11904, 12123, 12342, // 1024 - 1136 C
    12563, 12784, 13006, 13228, 13451, 13674, 13898, 14123, // 1152 - 1264 C
    14347, 14572, 14798, 15023, 15249, 15475, 15701, 15927, // 1280 - 1392 C
    16153, 16379, 16605, 16831, 17056, 17282, 17507, 17732, // 1408 - 1520 C
    17956, 18180, 18404, 18627, 18849, 19071, 19292, 19512, // 1536 - 1648 C
    19732, 19951, 20168, 20369, 20594, 20801, 21003, 21201	// 1664 - 1776 C
};

static const int16_t R_VALUES_CJ[] =
{
    -100, -76, -52, -26,   0,  27,  54,  83, // -20 - 15 C
     111, 141, 171, 201, 232, 264, 296, 329, //  20 - 55 C
     363, 397, 431                           //  60 - 70 C
 };

static const Thermocouple_Table R_TABLE    = {   0, 1776, 16, sizeof(R_VALUES   ) / sizeof(R_VALUES   [0]), R_VALUES    };
static const Thermocouple_Table R_TABLE_CJ = { -20,   70,  5, sizeof(R_VALUES_CJ) / sizeof(R_VALUES_CJ[0]), R_VALUES_CJ };

const Thermocouple_Type Thermocouple_TYPE_R = { &R_TABLE, &R_TABLE_CJ };

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static int16_t Table_C_to_uV(const Thermocouple_Table* aThis, int16_t aIn_C);
static int16_t Table_uV_to_C(const Thermocouple_Table* aThis, int16_t aIn_uV);

// Functions
// //////////////////////////////////////////////////////////////////////////

void Thermocouple_Init(Thermocouple* aThis, const Thermocouple_Type* aType)
{
    aThis->mCal_Offset_uV = 0;

    aThis->mType = aType;
}

uint8_t Thermocouple_uV_to_C(const Thermocouple* aThis, int16_t aTempCJ_C, int32_t aIn_uV, int16_t* aOut_C)
{
    int16_t lCJ_uV = Table_C_to_uV(aThis->mType->mTable_CJ, aTempCJ_C);

    int16_t lTemp_uV = aIn_uV + lCJ_uV + aThis->mCal_Offset_uV;

    const Thermocouple_Table* lTable = aThis->mType->mTable;

    int lResult = 0;

    if (lTable->mTable_uV[lTable->mCount - 1] < lTemp_uV)
    {
        lResult = 1;
    }

    *aOut_C = Table_uV_to_C(lTable, lTemp_uV);

    return lResult;
}

void Thermocouple_Calibrate(Thermocouple* aThis, int16_t aRead_C, int16_t aReal_C)
{
    const Thermocouple_Table* lTable = aThis->mType->mTable;

    int16_t lRead_uV = Table_C_to_uV(lTable, aRead_C) - aThis->mCal_Offset_uV;

    aThis->mCal_Offset_uV = Table_C_to_uV(lTable, aReal_C) - lRead_uV;
}

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

int16_t Table_C_to_uV(const Thermocouple_Table* aThis, int16_t aIn_C)
{
    int16_t lDelta_uV;
    int16_t lIndex;
    int16_t lInterpol_uV;

    const int16_t* lTable_uV = aThis->mTable_uV;

    if (aThis->mBegin_C >= aIn_C) { return lTable_uV[0                ]; }
    if (aThis->mEnd_C   <= aIn_C) { return lTable_uV[aThis->mCount - 1]; }

    lIndex = aIn_C / aThis->mStep_C;

    lDelta_uV = lTable_uV[lIndex + 1] - lTable_uV[lIndex];

    lInterpol_uV = (aIn_C % aThis->mStep_C) * lDelta_uV / aThis->mStep_C;

    return lTable_uV[lIndex] + lInterpol_uV;
}

int16_t Table_uV_to_C(const Thermocouple_Table* aThis, int16_t aIn_uV)
{
    uint16_t lA = 0;
    uint16_t lB = aThis->mCount - 1;
    int16_t  lDelta_uV;
    int16_t  lInterpol_C;

    const int16_t* lTable_uV = aThis->mTable_uV;

    if (lTable_uV[0                ] >= aIn_uV) { return aThis->mBegin_C; }
    if (lTable_uV[aThis->mCount - 1] <= aIn_uV) { return aThis->mEnd_C  ; }

    while (1 < (lB - lA))
    {
        uint16_t lC = (lA + lB) / 2;
        int16_t  lC_uV = lTable_uV[lC];

        if (aIn_uV == lC_uV) { return aThis->mBegin_C + (aThis->mStep_C * (int16_t)lC); }

        if (aIn_uV < lC_uV)
        {
            lB = lC;
        }
        else
        {
            lA = lC;
        }
    }

    lDelta_uV = lTable_uV[lB] - lTable_uV[lA];

    lInterpol_C = aThis->mStep_C * (aIn_uV - lTable_uV[lA]) / lDelta_uV;

    return aThis->mBegin_C + (aThis->mStep_C * (int16_t)lA) + lInterpol_C;
}
