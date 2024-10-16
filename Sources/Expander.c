
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

// ===== Includes ===========================================================
#include "I2C.h"
#include "I2C_Device.h"

#include "Expander.h"

// Data types
// //////////////////////////////////////////////////////////////////////////

// --> UNINITIALIZED
//     |
//     +--> IDLE <==+--------------------+
//          |       |                    |
//          +--> I2C_NEEDED <------------|-----------+
//               | | | |                 |           |
//               | | | +--> CONFIG       |           |
//               | | |      |            |           |
//               | | +------|--> INPUT   |           |
//               | |        |    |  |    |           |
//               | +--------|----|--+==> I2C_PENDING |
//               |          |    |       |           |
//               +----------+====+=======+=====> ERROR
typedef enum
{
    STATE_UNINITIALIZED = 0,
    
    STATE_CONFIG,
    STATE_ERROR,
    STATE_I2C_NEEDED,
    STATE_I2C_PENDING,
    STATE_IDLE,
    STATE_INPUT,
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

#define TIMEOUT_DISABLED (0)

#define TIMEOUT_ms (500)

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

static uint8_t sFlags_Running;

static uint8_t sInput [PORT_QTY];
static uint8_t sOutput[PORT_QTY];

static GPIO sInt;

static Expander_Callback sOnInputChanged;
static Expander_Callback sOnOperationCompleted;

static State sState = STATE_UNINITIALIZED;

static unsigned int sTimeout_ms;

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

static void SetState_ERROR();

static void Tick_CONFIG     (uint16_t aPeriod_ms);
static void Tick_I2C_NEEDED ();
static void Tick_I2C_PENDING(uint16_t aPeriod_ms);
static void Tick_INPUT      (uint16_t aPeriod_ms);

static void WriteConfig();

// Entry point
// //////////////////////////////////////////////////////////////////////////

void Expander_Interrupt();

#pragma interrupt alignsp saveall
void Expander_Interrupt()
{
    GPIO_Interrupt_Acknowledge(sInt);

    sFlags_Waiting |= FLAG_INPUT;
}

// Functions
// //////////////////////////////////////////////////////////////////////////

void Expander_Init(uint8_t aI2C, uint8_t aDevice, GPIO aInt, Expander_Callback aOnInputChanged)
{
    // assert(0x40 == (aDevice & 0xfc);

    I2C_Device_Init(&sDevice, aI2C, aDevice);

    sInt            = aInt;
    sOnInputChanged = aOnInputChanged;
    sState          = STATE_IDLE;

    GPIO_Init(sInt);

    GPIO_Interrupt_Enable(sInt);
}

void Expander_GPIO_Init(GPIO aDesc)
{
    uint8_t  lB;
    uint8_t  lB2;
    uint8_t  lM;
    uint8_t  lM2;
    uint16_t lP;
    uint16_t lP2;

    // assert(8        > aDesc.mBit);
    // assert(PORT_QTY > aDesc.mPort);

    lB  = 1 << aDesc.mBit;
    lB2 = 3 << (aDesc.mBit % 4 * 2);
    lM  = (uint8_t)(~ lB);
    lM2 = (uint8_t)(~ lB2);
    lP  = aDesc.mPort;
    lP2 = aDesc.mBit / 4;

    if (aDesc.mDrive) { sConfig_DriveStrength[lP][lP2] |= lB2; } else { sConfig_DriveStrength[lP][lP2] &= lM2; }

    if (aDesc.mPullUp_Enable) { sConfig_PullEnable[lP] |= lB ; } else { sConfig_PullEnable[lP] &= lM ; }
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

    GPIO_Interrupt_Disable(sInt);
    {
        sFlags_Waiting |= FLAG_CONFIG;
    }
    GPIO_Interrupt_Enable(sInt);
}

uint8_t Expander_GPIO_Input(GPIO aDesc)
{
    uint8_t  lB;
    uint16_t lP;

    // assert(8        > aDesc.mBit);
    // assert(PORT_QTY > aDesc.mPort);

    lB = 1 << aDesc.mBit;
    lP = aDesc.mPort;

    return 0 != (sInput[lP] & lB);
}

void Expander_GPIO_Output(GPIO aDesc, uint8_t aValue)
{
    uint8_t  lB;
    uint8_t  lM;
    uint16_t lP;

    // assert(8        > aDesc.mBit);
    // assert(PORT_QTY > aDesc.mPort);

    lB = 1 << aDesc.mBit;
    lM = (uint8_t)(~ lB);
    lP = aDesc.mPort;

    if (aValue)
    {
        sOutput[lP] |= lB;
    }
    else
    {
        sOutput[lP] &= lM;
    }

    GPIO_Interrupt_Disable(sInt);
    {
        sFlags_Waiting |= FLAG_OUTPUT;
    }
    GPIO_Interrupt_Enable(sInt);
}

uint8_t Expander_GPIO_Output_Get(GPIO aDesc)
{
    uint8_t  lB;
    uint16_t lP;

    // assert(8        > aDesc.mBit);
    // assert(PORT_QTY > aDesc.mPort);

    lB = 1 << aDesc.mBit;
    lP = aDesc.mPort;

    return 0 != (sOutput[lP] & lB);
}

void Expander_Tick(uint16_t aPeriod_ms)
{
    // assert(0 < aPeriod_ms);

    // assert(TIMEOUT_ms >= sTimeout_ms);
    
    if (TIMEOUT_DISABLED < sTimeout_ms)
    {
        if (sTimeout_ms > aPeriod_ms)
        {
            sTimeout_ms -= aPeriod_ms;
        }
        else
        {
            SetState_ERROR();
        }
    }

    switch (sState)
    {
    case STATE_ERROR:
    case STATE_IDLE:
        if (0 != sFlags_Waiting)
        {
            sTimeout_ms = TIMEOUT_ms;
            sState = STATE_I2C_NEEDED;
            Tick_I2C_NEEDED();
        }
        break;

    case STATE_CONFIG     : Tick_CONFIG     (aPeriod_ms); break;
    case STATE_I2C_PENDING: Tick_I2C_PENDING(aPeriod_ms); break;
    case STATE_INPUT      : Tick_INPUT      (aPeriod_ms); break;

    case STATE_I2C_NEEDED: Tick_I2C_NEEDED(); break;

    case STATE_UNINITIALIZED: break;
    }
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

void SetState_ERROR()
{
    // assert(TIMEOUT_ms >= sTimeout_ms);

    GPIO_Interrupt_Disable(sInt);
    {
        // Retry any running operation at next Tick
        sFlags_Waiting |= sFlags_Running;
    }
    GPIO_Interrupt_Enable(sInt);

    sFlags_Running = 0;
    sState         = STATE_ERROR;
    sTimeout_ms    = TIMEOUT_DISABLED;
}

void Tick_CONFIG(uint16_t aPeriod_ms)
{
    // assert(0 < aPeriod_ms)

    // assert(STATE_CONFIG == sState);

    I2C_Device_Tick(sDevice, aPeriod_ms);
    switch (I2C_Device_Status(sDevice))
    {
    case I2C_ERROR: SetState_ERROR(); break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        WriteConfig();

        if (CONFIG_OP_QTY <= sConfigIndex)
        {
            // We just changed the configuration, we maybe need to read input
            // status.
            GPIO_Interrupt_Disable(sInt);
            {
                sFlags_Waiting |= FLAG_INPUT;
            }
            GPIO_Interrupt_Enable(sInt);

            sState = STATE_I2C_PENDING;
        }
        break;

    // default: assert(false);
    }
}

void Tick_I2C_NEEDED()
{
    // assert(STATE_I2C_PENDING == sState);
    // assert(0 < sTimeout_ms);
    // assert(TIMEOUT_ms >= sTimeout_ms);

    if (I2C_Device_Idle(sDevice))
    {
        sTimeout_ms = 0;

        if (0 != (sFlags_Waiting & FLAG_CONFIG))
        {
            sFlags_Running = FLAG_CONFIG;
            sConfigIndex = 0;
            WriteConfig();
            sState = STATE_CONFIG;
        }
        else if (0 != (sFlags_Waiting & FLAG_OUTPUT))
        {
            sFlags_Running = FLAG_OUTPUT;
            I2C_Device_Write(sDevice, 0x02, sOutput, sizeof(sOutput));
            sState = STATE_I2C_PENDING;
        }
        else if (0 != (sFlags_Waiting & FLAG_INPUT))
        {
            sFlags_Running = FLAG_INPUT;
            I2C_Device_Write(sDevice, 0x00, NULL, 0);
            sState = STATE_INPUT;
        }
        else
        {
            sFlags_Running = 0;
            sState = STATE_IDLE;
        }

        GPIO_Interrupt_Disable(sInt);
        {
            sFlags_Waiting &= ~ sFlags_Running;
        }
        GPIO_Interrupt_Enable(sInt);
    }
}

void Tick_I2C_PENDING(uint16_t aPeriod_ms)
{
    // assert(0 < aPeriod_ms)

    // assert(0 != sFlags_Running);
    // assert(STATE_I2C_PENDING == sState);

    I2C_Device_Tick(sDevice, aPeriod_ms);
    switch (I2C_Device_Status(sDevice))
    {
    case I2C_ERROR: SetState_ERROR(); break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        sFlags_Running = 0;
        sState = STATE_IDLE;

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

    // assert(STATE_INPUT == sState);

    I2C_Device_Tick(sDevice, aPeriod_ms);
    switch (I2C_Device_Status(sDevice))
    {
    case I2C_ERROR: SetState_ERROR(); break;

    case I2C_PENDING: break;

    case I2C_SUCCESS:
        I2C_Device_Read(sDevice, sInput, sizeof(sInput));
        sOnOperationCompleted = sOnInputChanged;
        sState = STATE_I2C_PENDING;
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
