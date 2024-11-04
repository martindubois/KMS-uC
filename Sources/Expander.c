
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/Expander.c

// References
// //////////////////////////////////////////////////////////////////////////
//
// PCAL6416A Low-voltage translating 16-bit I2C-bus/SMBus I/O expander with
// interrupt output, reset, and configuration registers
// Rev. 7.1 — 30 August 2022
// https://www.nxp.com/docs/en/data-sheet/PCAL6416A.pdf

// Assumptions
// //////////////////////////////////////////////////////////////////////////

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////
//
// Project configuration
// - Also add MC56F/I2C.c to the project
//
// Processor Expert Configuration
// - Add an InterruptVector for INT_GPIOx associated to Expander_Interrupt
//   function

// Code
// //////////////////////////////////////////////////////////////////////////
//
// - Call I2C_Init

// ===== C ==================================================================
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ===== Includes ===========================================================
#include "I2C.h"
#include "I2C_Device.h"

#include "Expander.h"

// Data types
// //////////////////////////////////////////////////////////////////////////

// --> I2C_NEEDED <=========================+------------+
//     | | | | |                            |            |
//     | | | | +--> CONFIG                  |            |
//     | | | |      |  |                    |            |
//     | | | +------|--|--> INPUT           |            |
//     | | |        |  |    |  |            |            |
//     | | +--------|--|----|--|--> VERIFY  |            |
//     | |          |  |    |  |    |  |    |            |
//     | +----------|--+====|==+====|==+==> I2C_PENDING  |
//     |            |       |       |       |            |
//     +------------+=======+=======+=======+==> RESET --+
typedef enum
{
    STATE_I2C_NEEDED = 0,
    
    STATE_CONFIG,
    STATE_I2C_PENDING,
    STATE_INPUT,
    STATE_RESET,
    STATE_VERIFY,

    STATE_QTY
}
State;

typedef struct
{
    uint8_t  mAddress;
    uint8_t* mData;
    uint8_t  mSize_byte;
}
ConfigOp;

// Constants
// //////////////////////////////////////////////////////////////////////////

#define FLAG_CONFIG (0x01)
#define FLAG_INPUT  (0x02)
#define FLAG_OUTPUT (0x04)

#define PORT_QTY (2)

#define VERIFY_AGE_MAX_ms (1000)

// Variables
// //////////////////////////////////////////////////////////////////////////

static uint8_t sConfig_Configuration[PORT_QTY]    = { 0xff, 0xff };
static uint8_t sConfig_DriveStrength[PORT_QTY][2] = { { 0xff, 0xff }, { 0xff, 0xff } };
static uint8_t sConfig_PullEnable   [PORT_QTY]    = { 0x00, 0x00 };
static uint8_t sConfig_PullSelect   [PORT_QTY]    = { 0xff, 0xff };
static uint8_t sConfig_InterruptMask[PORT_QTY]    = { 0x00, 0x00 }; // Default = 0xff 0xff
static uint8_t sConfig_OutputPortConfig           = 0x00;

static uint8_t sConfigIndex;

static I2C_Device sDevice;

static uint8_t sInput        [PORT_QTY];
static uint8_t sInput_Default[PORT_QTY];
static uint8_t sOutput       [PORT_QTY];
static uint8_t sVerify       [PORT_QTY];

static uint16_t sVerifyAge_ms;

static GPIO sInt;
static GPIO sReset;

static Expander_Callback sOnInputChanged;
static Expander_Callback sOnOperationCompleted;

static State sState = STATE_I2C_NEEDED;

static uint16_t sStats[STATE_QTY];

static const ConfigOp CONFIG_OPS[] =
{
    { 0x4f, &sConfig_OutputPortConfig, sizeof(sConfig_OutputPortConfig) },
    { 0x4a, sConfig_InterruptMask    , sizeof(sConfig_InterruptMask   ) },
    { 0x48, sConfig_PullSelect       , sizeof(sConfig_PullSelect      ) },
    { 0x46, sConfig_PullEnable       , sizeof(sConfig_PullEnable      ) },
    { 0x42, sConfig_DriveStrength[1] , sizeof(sConfig_DriveStrength[1]) },
    { 0x40, sConfig_DriveStrength[0] , sizeof(sConfig_DriveStrength[0]) },
    { 0x06, sConfig_Configuration    , sizeof(sConfig_Configuration   ) },
};

#define CONFIG_OP_QTY (sizeof(CONFIG_OPS) / sizeof(CONFIG_OPS[0]))

// ===== Zone 0 =============================================================
// Interrupt  Read / Write
// Tick       Read / Write
// Protected using GPIO_Interrupt_Disable and GPIO_Interrupt_Enable
static uint8_t sFlags_Waiting;

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static void AddFlags(uint8_t aFlags);

static void CopyDefaultInput();

static void OnConfigCompleted();
static void OnVerifyChanged();

static void SetState(State aState);

static void SetState_CONFIG     ();
static void SetState_I2C_PENDING(Expander_Callback aOnCompletion);
static void SetState_INPUT      ();
static void SetState_RESET      ();
static void SetState_VERIFY     ();

static void Tick_CONFIG     (uint16_t aPeriod_ms);
static void Tick_I2C_NEEDED ();
static void Tick_I2C_PENDING(uint16_t aPeriod_ms);
static void Tick_INPUT      (uint16_t aPeriod_ms);
static void Tick_RESET      ();
static void Tick_VERIFY     (uint16_t aPeriod_ms);

static void WriteConfig();

// Entry point
// //////////////////////////////////////////////////////////////////////////

void Expander_Interrupt();

#pragma interrupt alignsp saveall
void Expander_Interrupt()
{
    GPIO_Interrupt_Acknowledge(sInt);

    sFlags_Waiting |= FLAG_INPUT;

    // TODO  We know the expander did not reset, because interruption are
    //       masked at reset. We could reset the verification timer here.
    //       But if noise can generate interrupt, we don't want to reset the
    //       verification timer here.
}

// Functions
// //////////////////////////////////////////////////////////////////////////

void Expander_Init(uint8_t aI2C, uint8_t aDevice, GPIO aReset, const uint8_t* aDefaultInput, GPIO aInt, Expander_Callback aOnInputChanged)
{
    // assert(0x40 == (aDevice & 0xfc);
    // assert(NULL != aDefaultInput);

    I2C_Device_Init(&sDevice, aI2C, aDevice);

    sInt            = aInt;
    sOnInputChanged = aOnInputChanged;
    sReset          = aReset;

    sInt.mInterrupt_Falling = 1;
    sInt.mOutput            = 0;

    sReset.mOutput        = 1;
    sReset.mSlewRate_Slow = 1;

    memcpy(&sInput_Default, aDefaultInput, sizeof(sInput_Default));

    CopyDefaultInput();

    GPIO_Init(sInt);
    GPIO_Init(sReset);

    GPIO_Interrupt_Enable(sInt);

    GPIO_Output(sReset, 0);
    GPIO_Output(sReset, 1);
}

void Expander_GPIO_Init(GPIO aDesc)
{
    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        // assert(       8 > aDesc.mBit);
        // assert(PORT_QTY > aDesc.mPort);

        uint8_t  lB;
        uint8_t  lB2;
        uint8_t  lM;
        uint8_t  lM2;
        uint16_t lP;
        uint16_t lP2;

        lB  = 1 << aDesc.mBit;
        lB2 = 3 << (aDesc.mBit % 4 * 2);
        lM  = (uint8_t)(~ lB);
        lM2 = (uint8_t)(~ lB2);
        lP  = aDesc.mPort;
        lP2 = aDesc.mBit / 4;

        if (aDesc.mDrive) { sConfig_DriveStrength[lP][lP2] |= lB2; } else { sConfig_DriveStrength[lP][lP2] &= lM2; }

        if (aDesc.mPull_Enable  ) { sConfig_PullEnable[lP] |= lB ; } else { sConfig_PullEnable[lP] &= lM ; }
        if (aDesc.mPullUp_Select) { sConfig_PullSelect[lP] |= lB ; } else { sConfig_PullSelect[lP] &= lM ; }

        if (aDesc.mOutput)
        {
            if (aDesc.mPushPull) { sConfig_OutputPortConfig &= ~ (1 << lP); } else { sConfig_OutputPortConfig |= 1 << lP; }

            sConfig_Configuration[lP] &= lM;
        }
        else
        {
            sConfig_Configuration[lP] |= lB;
        }

        AddFlags(FLAG_CONFIG);
    }
}

uint8_t Expander_GPIO_Input(GPIO aDesc)
{
    uint8_t lResult = 0;

    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        // assert(8        > aDesc.mBit);
        // assert(PORT_QTY > aDesc.mPort);

        uint8_t  lB;
        uint16_t lP;

        lB = 1 << aDesc.mBit;
        lP = aDesc.mPort;

        lResult = (0 != (sInput[lP] & lB));
    }

    return lResult;
}

void Expander_GPIO_Output(GPIO aDesc, uint8_t aValue)
{
    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        // assert(8        > aDesc.mBit);
        // assert(PORT_QTY > aDesc.mPort);

        uint8_t  lB;
        uint8_t  lBefore;
        uint8_t  lM;
        uint16_t lP;

        lB = 1 << aDesc.mBit;
        lM = (uint8_t)(~ lB);
        lP = aDesc.mPort;

        lBefore = sOutput[lP];

        if (aValue)
        {
            sOutput[lP] |= lB;
        }
        else
        {
            sOutput[lP] &= lM;
        }

        if (lBefore != sOutput[lP])
        {
            AddFlags(FLAG_OUTPUT);
        }
    }
}

uint8_t Expander_GPIO_Output_Get(GPIO aDesc)
{
    uint8_t lResult = 0;

    if (GPIO_PORT_DUMMY > aDesc.mPort)
    {
        // assert(8        > aDesc.mBit);
        // assert(PORT_QTY > aDesc.mPort);

        uint8_t  lB;
        uint16_t lP;

        lB = 1 << aDesc.mBit;
        lP = aDesc.mPort;

        lResult = (0 != (sOutput[lP] & lB));
    }

    return lResult;
}

void Expander_Tick(uint16_t aPeriod_ms)
{
    // assert(0 < aPeriod_ms);

    sVerifyAge_ms += aPeriod_ms;

    switch (sState)
    {
    case STATE_CONFIG     : Tick_CONFIG     (aPeriod_ms); break;
    case STATE_I2C_PENDING: Tick_I2C_PENDING(aPeriod_ms); break;
    case STATE_INPUT      : Tick_INPUT      (aPeriod_ms); break;
    case STATE_VERIFY     : Tick_VERIFY     (aPeriod_ms); break;

    case STATE_I2C_NEEDED: Tick_I2C_NEEDED(); break;
    case STATE_RESET     : Tick_RESET     (); break;

    // default: assert(false);
    }
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

void AddFlags(uint8_t aFlags)
{
    // assert(0 != aFlags);

    GPIO_Interrupt_Disable(sInt);
    {
        sFlags_Waiting |= aFlags;
    }
    GPIO_Interrupt_Enable(sInt);
}

void CopyDefaultInput()
{
    memcpy(&sInput, &sInput_Default, sizeof(sInput));
}

void OnConfigCompleted()
{
    // We just completed the configuration, we need to read input and update
    // output.
    AddFlags(FLAG_INPUT | FLAG_OUTPUT);
}

void OnVerifyChanged()
{
    unsigned int i;

    for (i = 0; i < PORT_QTY; i++)
    {
        if (sConfig_InterruptMask[i] != sVerify[i])
        {
            SetState_RESET();
            break;
        }
    }
}

void SetState(State aState)
{
    // assert(STATE_QTY > aState);

    sState = aState;

    sStats[sState]++;
}

void SetState_CONFIG()
{
    sConfigIndex = 0;
    WriteConfig();

    SetState(STATE_CONFIG);
}

void SetState_I2C_PENDING(Expander_Callback aOnCompletion)
{
    sOnOperationCompleted = aOnCompletion;

    SetState(STATE_I2C_PENDING);
}

void SetState_INPUT()
{
    I2C_Device_Write(sDevice, 0x00, NULL, 0);

    SetState(STATE_INPUT);
}

void SetState_RESET()
{
    GPIO_Output(sReset, 0);

    SetState(STATE_RESET);

    CopyDefaultInput();

    if (NULL != sOnInputChanged)
    {
        sOnInputChanged();
    }
}

void SetState_VERIFY()
{
    // assert(0 < sVerifyAge_ms);

    I2C_Device_Write(sDevice, 0x4a, NULL, 0);

    SetState(STATE_VERIFY);

    sVerifyAge_ms = 0;
}

void Tick_CONFIG(uint16_t aPeriod_ms)
{
    // assert(0 < aPeriod_ms)

    I2C_Device_Tick(sDevice, aPeriod_ms);
    switch (I2C_Device_Status(sDevice))
    {
    case I2C_ERROR: SetState_RESET(); break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        WriteConfig();

        if (CONFIG_OP_QTY <= sConfigIndex)
        {
            SetState_I2C_PENDING(OnConfigCompleted);
        }
        break;

    // default: assert(false);
    }
}

void Tick_I2C_NEEDED()
{
    if (I2C_Device_Idle(sDevice))
    {
        if ((0 == sFlags_Waiting) || (VERIFY_AGE_MAX_ms <= sVerifyAge_ms))
        {
            SetState_VERIFY();
        }
        else
        {
            uint8_t lFlags_Running = 0;

            if (0 != (sFlags_Waiting & FLAG_CONFIG))
            {
                lFlags_Running = FLAG_CONFIG;
                SetState_CONFIG();
            }
            else
            {
                static uint8_t sToggle;

                if (sToggle)
                {
                    if (0 != (sFlags_Waiting & FLAG_INPUT))
                    {
                        lFlags_Running = FLAG_INPUT;
                        SetState_INPUT();
                    }

                    sToggle = 0;
                }
                else
                {
                    if (0 != (sFlags_Waiting & FLAG_OUTPUT))
                    {
                        lFlags_Running = FLAG_OUTPUT;

                        I2C_Device_Write(sDevice, 0x02, sOutput, sizeof(sOutput));

                        SetState_I2C_PENDING(NULL);
                    }

                    sToggle = 1;
                }
            }

            if (0 != lFlags_Running)
            {
                GPIO_Interrupt_Disable(sInt);
                {
                    sFlags_Waiting &= ~ lFlags_Running;
                }
                GPIO_Interrupt_Enable(sInt);
            }
        }
    }
}

void Tick_I2C_PENDING(uint16_t aPeriod_ms)
{
    // assert(0 < aPeriod_ms)

    I2C_Device_Tick(sDevice, aPeriod_ms);
    switch (I2C_Device_Status(sDevice))
    {
    case I2C_ERROR: SetState_RESET(); break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        SetState(STATE_I2C_NEEDED);

        if (NULL != sOnOperationCompleted)
        {
            sOnOperationCompleted();
            sOnOperationCompleted = NULL;
        }
        break;

    // default: assert(false);
    }
}

void Tick_INPUT(uint16_t aPeriod_ms)
{
    // assert(0 < aPeriod_ms)

    I2C_Device_Tick(sDevice, aPeriod_ms);
    switch (I2C_Device_Status(sDevice))
    {
    case I2C_ERROR: SetState_RESET(); break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        I2C_Device_Read(sDevice, sInput, sizeof(sInput));

        SetState_I2C_PENDING(sOnInputChanged);
        break;

    // default: assert(false);
    }
}

void Tick_RESET()
{
    GPIO_Output(sReset, 1);

    SetState(STATE_I2C_NEEDED);

    AddFlags(FLAG_CONFIG);
}

void Tick_VERIFY(uint16_t aPeriod_ms)
{
    // assert(0 < aPeriod_ms);

    I2C_Device_Tick(sDevice, aPeriod_ms);
    switch (I2C_Device_Status(sDevice))
    {
    case I2C_ERROR: SetState_RESET(); break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        I2C_Device_Read(sDevice, sVerify, sizeof(sVerify));

        SetState_I2C_PENDING(OnVerifyChanged);
        break;

    // default: assert(false);
    }
}

void WriteConfig()
{
    // assert(CONFIG_OP_QTY > sConfigIndex);

    const ConfigOp* lOp = CONFIG_OPS + sConfigIndex;
    // assert(NULL != lOp->mData);
    // assert(0 < lOp->mSize_byte);

    I2C_Device_Write(sDevice, lOp->mAddress, lOp->mData, lOp->mSize_byte);

    sConfigIndex++;
}
