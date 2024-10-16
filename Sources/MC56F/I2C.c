
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/MC56F/I2C.c

// References
// //////////////////////////////////////////////////////////////////////////
//
// MC56F8458x Reference Manual with Addendum
// https://www.nxp.com/docs/en/reference-manual/MC56F8458XRM.pdf
// Chapter 37 - Inter-Integrated Circuit (I2C)

// Assumptions
// //////////////////////////////////////////////////////////////////////////

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////
//
// Project configuration
// - Define _MC56F84565_
//
// Processor expert configuration
// - Add an InterruptVector INT_IIC0 or/and INT_IIC1 associated to
//   I2C0_Interrupt or/and I2C1_Interrupt

// Code
// //////////////////////////////////////////////////////////////////////////

// ===== C ==================================================================
#include <stdint.h>

// ===== Includes ===========================================================
#include "GPIO.h"
#include "MC56F_SIM.h"

#include "I2C.h"

// Data types
// //////////////////////////////////////////////////////////////////////////

// --> IDLE <===============+----------------+
//     |                    |                |
//     +--> TX_DEVICE  +--> COMPLETED <---+  |
//          | | |      |                  |  |
//          | | +--> RX_DATA              |  |
//          | |            |              |  |
//          | +--> TX_ADDR |              |  |
//          |      | |     |              |  |
//          |      | +-----|--> TX_DATA --+  |
//          |      |       |    |            |
//          +------+=======+====+==> ERROR --+
typedef enum
{
    STATE_IDLE = 0,

    STATE_COMPLETED,
    STATE_ERROR,
    STATE_RX_DATA,
    STATE_TX_ADDR,
    STATE_TX_DATA,
    STATE_TX_DEVICE,

    STATE_QTY
}
State;

typedef struct
{
    uint8_t mIndex;

    uint8_t      mAddress;
    uint8_t    * mDataPtr;
    unsigned int mDataSize_byte;
    uint8_t      mDevice;
    State        mState;
    unsigned int mTimeout_ms;

    uint16_t     mDummy;
}
Context;

typedef struct
{
    uint16_t mAddress1;
    uint16_t mFreqDiv;
    uint16_t mControl1;
    uint16_t mStatus;
    uint16_t mData;
    uint16_t mControl2;
    uint16_t mGlitchFilter;
    uint16_t mRangeAddress;
    uint16_t mSMBus;
    uint16_t mAddress2;
    uint16_t mTimeout_High;
    uint16_t mTimeout_Low;

    uint16_t mReserved0[4];
}
PortRegs;

#define C1_TXAK  (0x0008)
#define C1_TX    (0x0010)
#define C1_MST   (0x0020)
#define C1_IICIE (0x0040)
#define C1_IICEN (0x0080)

#define S_RXAK  (0x0001)
#define S_IICIF (0x0002)
#define S_ARBL  (0x0010)
#define S_TCF   (0x0080)

typedef struct
{
    GPIO mSDA;
    GPIO mSCL;
}
GPIOs;

// Constants
// //////////////////////////////////////////////////////////////////////////

#define I2C_READ_BIT (0x01)

#define I2C_QTY (2)

static volatile PortRegs* PORT_REGS = (PortRegs*)0x0000E0E0;

static GPIOs GPIO_TABLE[I2C_QTY];

static const uint16_t SIM_PCE1_BITS[I2C_QTY] = { 0x0040, 0x0020 };

#define TIMEOUT_ms (100)

// Variables
// //////////////////////////////////////////////////////////////////////////

static Context sContexts[I2C_QTY];

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static void Interrupt(Context* aThis);

static void Interrupt_Disable(Context* aThis);
static void Interrupt_Enable (Context* aThis);

static void Interrupt_RX_DATA  (Context* aThis);
static void Interrupt_TX_ADDR  (Context* aThis, uint16_t aStatus);
static void Interrupt_TX_DATA  (Context* aThis, uint16_t aStatus);
static void Interrupt_TX_DEVICE(Context* aThis, uint16_t aStatus);

static void SetState_ERROR    (Context* aThis);
static void SetState_COMPLETED(Context* aThis);

static void Start_TX_DEVICE(Context* aThis);

// Entry points
// //////////////////////////////////////////////////////////////////////////

void I2C0_Interrupt();
void I2C1_Interrupt();

#pragma interrupt alignsp saveall
void I2C0_Interrupt() { Interrupt(sContexts + 0); }

#pragma interrupt alignsp saveall
void I2C1_Interrupt() { Interrupt(sContexts + 1); }

// Functions
// //////////////////////////////////////////////////////////////////////////

void I2Cs_Init0()
{
    uint8_t i;

    GPIO_TABLE[0].mSCL.mBit = 15;
    GPIO_TABLE[0].mSDA.mBit = 14;

    GPIO_TABLE[1].mSCL.mBit      = 11;
    GPIO_TABLE[1].mSCL.mFunction =  1;
    GPIO_TABLE[1].mSDA.mBit      = 12;
    GPIO_TABLE[1].mSDA.mFunction =  1;

    for (i = 0; i < I2C_QTY; i++)
    {
        GPIO_TABLE[i].mSCL.mPort = GPIO_PORT_C;
        GPIO_TABLE[i].mSDA.mPort = GPIO_PORT_C;

        sContexts[i].mIndex = i;
    }    
}

void I2C_Init(uint8_t aIndex)
{
    // assert(I2C_QTY > aIndex);

    volatile PortRegs* lR = PORT_REGS + aIndex;

    GPIO_InitFunction(GPIO_TABLE[aIndex].mSDA);
    GPIO_InitFunction(GPIO_TABLE[aIndex].mSCL);

    *SIM_PCE1 |= SIM_PCE1_BITS[aIndex];

    lR->mFreqDiv      = 0x0094; // MULT = 4 | ICR = 20
    lR->mControl2     = 0;
    lR->mAddress1     = 0;
    lR->mTimeout_High = 0;
    lR->mTimeout_Low  = 0;
    lR->mAddress2     = 0;
    lR->mSMBus        = 0;
    lR->mGlitchFilter = 0x0053; // STOPF | STARTF | FLT = 3
    lR->mRangeAddress = 0;
}

uint8_t I2C_Idle(uint8_t aIndex)
{
    // assert(I2C_QTY > aIndex);
    // assert(STATE_QTY > sContexts[aIndex].mState);

    return STATE_IDLE == sContexts[aIndex].mState;
}

uint8_t I2C_Status(uint8_t aIndex)
{
    // assert(I2C_QTY > aIndex);

    uint8_t  lResult = I2C_ERROR;
    Context* lThis   = sContexts + aIndex;

    switch (lThis->mState)
    {
    case STATE_COMPLETED:
        lResult = I2C_SUCCESS;
        lThis->mState = STATE_IDLE;
        break;

    case STATE_ERROR:
        lThis->mState = STATE_IDLE;
        break;

    case STATE_RX_DATA:
    case STATE_TX_ADDR:
    case STATE_TX_DEVICE:
        lResult = I2C_PENDING;
        break;

    // default: assert(false);
    }

    return lResult;
}

void I2C_Read(uint8_t aIndex, uint8_t aDevice, void* aOut, uint8_t aOutSize_byte)
{
    // assert(I2C_QTY > aIndex);
    // assert(0 == (aDevice & I2C_READ_BIT));
    // assert(NULL != aOut);
    // assert(0 < aOutSize_byte);

    Context* lThis = sContexts + aIndex;

    lThis->mDataPtr       = aOut;
    lThis->mDataSize_byte = aOutSize_byte;
    lThis->mDevice        = aDevice | I2C_READ_BIT;

    Start_TX_DEVICE(lThis);
}

void I2C_Write(uint8_t aIndex, uint8_t aDevice, uint8_t aAddress, const void* aIn, uint8_t aInSize_byte)
{
    // assert(I2C_QTY > aIndex);
    // assert(0 == (aDevice & I2C_READ_BIT));

    Context* lThis = sContexts + aIndex;

    lThis->mAddress       = aAddress;
    lThis->mDataPtr       = (void*)aIn;
    lThis->mDataSize_byte = aInSize_byte;
    lThis->mDevice        = aDevice;

    Start_TX_DEVICE(lThis);
}

void I2C_Tick(uint8_t aIndex, uint16_t aPeriod_ms)
{
    // assert(I2C_QTY > aIndex);

    Context* lThis = sContexts + aIndex;

    Interrupt_Disable(lThis);
    {
        switch (lThis->mState)
        {
        case STATE_COMPLETED:
        case STATE_ERROR:
        case STATE_IDLE:
            // assert(0 == lThis->mTimeout_ms);
            break;

        case STATE_RX_DATA:
        case STATE_TX_ADDR:
        case STATE_TX_DATA:
        case STATE_TX_DEVICE:
            // assert(0 < lThis->mTimeout_ms);

            if (lThis->mTimeout_ms <= aPeriod_ms)
            {
                SetState_ERROR(lThis);
            }
            else
            {
                lThis->mTimeout_ms -= aPeriod_ms;
            }
            break;

        // default: assert(false);
        }
    }
    Interrupt_Enable(lThis);
    
}

// Static functions
// //////////////////////////////////////////////////////////////////////////

void Interrupt(Context* aThis)
{
    // assert(I2C_QTY > aThis->mIndex);
    // assert(STATE_QTY > aThis->mState);

    volatile PortRegs* lR = PORT_REGS + aThis->mIndex;
    uint16_t           lStatus;

    lStatus = lR->mStatus;

    if (0 == (lStatus & S_ARBL))
    {
        if (0 != (lStatus & S_TCF))
        {
            switch (aThis->mState)
            {
            case STATE_COMPLETED:
            case STATE_ERROR:
            case STATE_IDLE:
                aThis->mDummy = lR->mData;
                break;

            case STATE_RX_DATA  : Interrupt_RX_DATA  (aThis); break;
            case STATE_TX_ADDR  : Interrupt_TX_ADDR  (aThis, lStatus); break;
            case STATE_TX_DATA  : Interrupt_TX_DATA  (aThis, lStatus); break;
            case STATE_TX_DEVICE: Interrupt_TX_DEVICE(aThis, lStatus); break;

            // default: assert(false);
            }
        }
        else
        {
            SetState_ERROR(aThis);
        }
    }
    else
    {
        // Arbitration lost

        lR->mStatus = S_ARBL;
        if (0 != (lStatus & S_TCF))
        {
            aThis->mDummy = lR->mData;
        }

        switch (aThis->mState)
        {
        case STATE_IDLE: break;

        default: SetState_ERROR(aThis);
        }
    }

    // Clear the interrupt
    lR->mStatus = S_IICIF;
}

void Interrupt_Disable(Context* aThis)
{
    // assert(I2C_QTY > aThis->mIndex);

    volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

    lR->mControl1 &= ~ C1_IICIE;
}

void Interrupt_Enable(Context* aThis)
{
    // assert(I2C_QTY > aThis->mIndex);

    volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

    lR->mControl1 |= C1_IICIE;
}

void Interrupt_RX_DATA(Context* aThis)
{
    // assert(NULL != aThis->mDataPtr);
    // assert(0 < aThis->mDataSize_byte);
    // assert(I2C_QTY > aThis->mIndex);

    volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

    if (1 >= aThis->mDataSize_byte)
    {
        lR->mControl1 |= C1_TXAK;
    }

    *aThis->mDataPtr = (uint8_t)lR->mData;

    aThis->mDataPtr++;
    aThis->mDataSize_byte--;

    if (0 >= aThis->mDataSize_byte)
    {
        SetState_COMPLETED(aThis);
    }
}

void Interrupt_TX_ADDR(Context* aThis, uint16_t aStatus)
{
    // assert(I2C_QTY > aThis->mIndex);
    // assert(STATE_TX_ADDR == aThis->mState);

    aThis->mState = STATE_TX_DATA;

    Interrupt_TX_DATA(aThis, aStatus);
}

void Interrupt_TX_DATA(Context* aThis, uint16_t aStatus)
{
    // assert(I2C_QTY > aThis->mIndex);

    if (0 == (aStatus & S_RXAK))
    {
        if (0 < aThis->mDataSize_byte)
        {
            // assert(NULL != aThis->mDataPtr);

            volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

            lR->mData = (uint16_t)*aThis->mDataPtr;

            aThis->mDataPtr++;
            aThis->mDataSize_byte--;
        }
        else
        {
            SetState_COMPLETED(aThis);
        }
    }
    else
    {
        SetState_ERROR(aThis);
    }
}

void Interrupt_TX_DEVICE(Context* aThis, uint16_t aStatus)
{
    // assert(I2C_QTY > aThis->mIndex);
    // assert(STATE_TX_DVICE == aThis->mState);

    if (0 == (aStatus & S_RXAK))
    {
        volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

        if (0 == (aThis->mDevice & I2C_READ_BIT))
        {
            lR->mData = aThis->mAddress;

            aThis->mState = STATE_TX_ADDR;
        }
        else
        {
            uint16_t lDummy;
        
            lR->mControl1 &= ~ C1_TX;
            lR->mControl1 &= ~ C1_TXAK;

            lDummy = lR->mData;

            aThis->mState = STATE_RX_DATA;
        }
    }
    else
    {
        SetState_ERROR(aThis);
    }
}

void SetState_COMPLETED(Context* aThis)
{
    // assert(I2C_QTY > aThis->mIndex);
    // assert((STATE_RX_DATA == aThis->mState) || (STATE_TX_DATA == aThis->mState));
    // assert(0 < aThis->mTimeout_ms);

    volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

    lR->mControl1 &= ~ C1_TX;
    lR->mControl1 &= ~ C1_MST;

    aThis->mState = STATE_COMPLETED;
    aThis->mTimeout_ms = 0;
}

void SetState_ERROR(Context* aThis)
{
    // assert(I2C_QTY > aThis->mIndex);
    // assert(STATE_QTY > aThis->mState);
    // assert(0 < aThis->mTimeout_ms);

    volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

    lR->mControl1 &= ~ (C1_MST | C1_TX | C1_TXAK);

    aThis->mState = STATE_ERROR;
    aThis->mTimeout_ms = 0;
}

void Start_TX_DEVICE(Context* aThis)
{
    // assert(I2C_QTY > aThis->mIndex);
    // assert((STATE_ERROR == aThis->mState) || (STATE_IDLE == aThis->mState));
    
    volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

    lR->mControl1 = 0;

    lR->mStatus = S_ARBL | S_IICIF;

    lR->mControl1 = C1_IICEN | C1_IICIE | C1_TX | C1_TXAK | C1_MST;

    aThis->mState = STATE_TX_DEVICE;
    aThis->mTimeout_ms = TIMEOUT_ms;

    lR->mData = (uint16_t)aThis->mDevice;
}
