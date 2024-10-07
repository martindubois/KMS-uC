
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Sources/MC56F/PWMA.c

// Assumption
// //////////////////////////////////////////////////////////////////////////
//
// - The system clock (IP Bus) is 80 MHz

// CodeWarrior
// //////////////////////////////////////////////////////////////////////////
//
// Project configuration
// - Define _MC56F84565_
//
// Processor Expert Configuration

// Code
// //////////////////////////////////////////////////////////////////////////
//
// - Configure Input/Output pin using GPIO_InitFunction

// ===== C ==================================================================
#include <stdint.h>
#include <stdlib.h>

// ===== Includes ===========================================================
#include "MC56F_SIM.h"

#include "UART.h"

// Data types
// //////////////////////////////////////////////////////////////////////////

// --> IDLE <==+===+======+==============+---------+
//     | |     |   |      |              |         |
//     | +--> RX   |      |              |         |
//     |      |    |      |              |         |
//     +------|--> TX --> TX_WAIT --+==> COMPLETED |
//            |    |      |              |         |
//            +----+======+==============+===> ERROR
typedef enum
{
    STATE_COMPLETED = 0,
    STATE_ERROR,
    STATE_IDLE,
    STATE_RX,
    STATE_TX,
    STATE_TX_WAIT,

    STATE_QTY
}
State;

typedef struct
{
    uint16_t mRate;
    uint16_t mCtrl1;
    uint16_t mCtrl2;
    uint16_t mStatus;
    uint16_t mData;
    uint16_t mCtrl3;

    uint16_t mReserved0[10];
}
PortRegs;

#define CTRL1_RFIE (0x0010)
#define CTRL1_REIE (0x0020)
#define CTRL1_TIIE (0x0040)
#define CTRL1_TEIE (0x0080)

typedef struct
{
    uint8_t* mInOut;

    uint16_t mTimeout_ms;

    uint8_t mCount;
    uint8_t mSize_byte;
    uint8_t mState;
}
HalfContext;

#define OP_QTY (2)

typedef struct
{
    uint8_t mIndex;

    uint16_t mEnabledInterrupts;

    HalfContext mContexts[OP_QTY];
}
Context;

// Constants
// //////////////////////////////////////////////////////////////////////////

#define WRITE_TIMEOUT_ms_byte (2)

#define QSCI_QTY (3)

static volatile PortRegs * PORT_REGS = (PortRegs*)0x0000e080;

static const uint16_t SIM_PCE1_BITS[QSCI_QTY] = { 0x1000, 0x0800, 0x0400 };

// Variables
// //////////////////////////////////////////////////////////////////////////

static Context sContexts[QSCI_QTY];

// Static function declarations
// //////////////////////////////////////////////////////////////////////////

static void IncCount_Z0(HalfContext* aThisH, State aNext, uint16_t aTimeout_ms);

static void Interrupt_Disable(uint8_t aIndex);
static void Interrupt_Enable (uint8_t aIndex);

static void Interrupt_RCV_Z0  (Context* aThis);
static void Interrupt_RERR_Z0 (Context* aThis);
static void Interrupt_TIDLE_Z0(Context* aThis);
static void Interrupt_TDRE_Z0 (Context* aThis);

static void Receive_Z0(Context* aThis);

static void Send_Z0(Context* aThis);

static void Start_Z0(HalfContext* aThisH, void* aInOut, uint8_t aSize_byte);

// Entry points
// //////////////////////////////////////////////////////////////////////////

void SCI0_Interrupt_RERR ();
void SCI0_Interrupt_RCV  ();
void SCI0_Interrupt_TIDLE();
void SCI0_Interrupt_TDRE ();
void SCI1_Interrupt_RERR ();
void SCI1_Interrupt_RCV  ();
void SCI1_Interrupt_TIDLE();
void SCI1_Interrupt_TDRE ();
void SCI2_Interrupt_RERR ();
void SCI2_Interrupt_RCV  ();
void SCI2_Interrupt_TIDLE();
void SCI2_Interrupt_TDRE ();

#pragma interrupt alignsp saveall
void SCI0_Interrupt_RERR() { Interrupt_RERR_Z0(sContexts + 0); }

#pragma interrupt alignsp saveall
void SCI0_Interrupt_RCV() { Interrupt_RCV_Z0(sContexts + 0); }

#pragma interrupt alignsp saveall
void SCI0_Interrupt_TIDLE() { Interrupt_TIDLE_Z0(sContexts + 0); }

#pragma interrupt alignsp saveall
void SCI0_Interrupt_TDRE() { Interrupt_TDRE_Z0(sContexts + 0); }

#pragma interrupt alignsp saveall
void SCI1_Interrupt_RERR() { Interrupt_RERR_Z0(sContexts + 1); }

#pragma interrupt alignsp saveall
void SCI1_Interrupt_RCV() { Interrupt_RCV_Z0(sContexts + 1); }

#pragma interrupt alignsp saveall
void SCI1_Interrupt_TIDLE() { Interrupt_TIDLE_Z0(sContexts + 1); }

#pragma interrupt alignsp saveall
void SCI1_Interrupt_TDRE() { Interrupt_TDRE_Z0(sContexts + 1); }

#pragma interrupt alignsp saveall
void SCI2_Interrupt_RERR() { Interrupt_RERR_Z0(sContexts + 2); }

#pragma interrupt alignsp saveall
void SCI2_Interrupt_RCV() { Interrupt_RCV_Z0(sContexts + 2); }

#pragma interrupt alignsp saveall
void SCI2_Interrupt_TIDLE() { Interrupt_TIDLE_Z0(sContexts + 2); }

#pragma interrupt alignsp saveall
void SCI2_Interrupt_TDRE() { Interrupt_TDRE_Z0(sContexts + 2); }

// Functions
// //////////////////////////////////////////////////////////////////////////

void UART_Init(uint8_t aIndex)
{
    // assert(QSCI_QTY > aIndex);

    volatile PortRegs* lR = PORT_REGS + aIndex;

    Context* lThis = sContexts + aIndex;

    unsigned int i;

    lThis->mEnabledInterrupts = CTRL1_REIE | CTRL1_RFIE;
    lThis->mIndex             = aIndex;

    for (i = 0; i < OP_QTY; i++)
    {
        HalfContext* lThisH = lThis->mContexts + i;

        lThisH->mCount      = 0;
        lThisH->mInOut      = NULL;
        lThisH->mSize_byte  = 0;
        lThisH->mState      = STATE_IDLE;
        lThisH->mTimeout_ms = 0;
    }

    *SIM_PCE1 |= SIM_PCE1_BITS[aIndex];

    lR->mCtrl2 = 0x0020; // FIFO_EN 

    // https://www.nxp.com/docs/en/reference-manual/MC56F8458XRM.pdf, page 956
    // Baud rate = Peripheral bus clock / ( 16 * ( SBR + ( FRAC / 8 ) ) )

    // = 80 MHz / ( 16 * ( 260 + ( 3 / 8 ) ) )
    // = 80 MHz / ( 16 * 260.375 )
    // = 80 MHz / 4166
    // = 19.2031 bps (0.52 ms/byte)

    lR->mRate = 0x0823; // SBR = 1000 0010 0 = 260, FRAC = 3

    Interrupt_Enable(aIndex);
}

void UART_Abort(uint8_t aIndex, uint8_t aOp)
{
    // assert(QSCI_QTY > aIndex);
    // assert(OP_QTY > aOp);

    HalfContext* lThisH = sContexts[aIndex].mContexts + aOp;

    Interrupt_Disable(aIndex);
    {
        lThisH->mCount      = 0;
        lThisH->mInOut      = NULL;
        lThisH->mSize_byte  = 0;
        lThisH->mState      = STATE_IDLE;
        lThisH->mTimeout_ms = 0;
    }
    Interrupt_Enable(aIndex);
}

uint8_t UART_Idle(uint8_t aIndex, uint8_t aOp)
{
    // assert(QSCI_QTY > aIndex);
    // assert(OP_QTY > aOp);

    HalfContext* lThisH = &sContexts[aIndex].mContexts[aOp];

    return STATE_IDLE == lThisH->mState;
}

void UART_SetTimeout(uint8_t aIndex, uint8_t aOp, uint16_t aTimeout_ms)
{
    // assert(QSCI_QTY > aIndex);
    // assert(OP_QTY > aOp);

    HalfContext* lThisH = &sContexts[aIndex].mContexts[aOp];

    lThisH->mTimeout_ms = aTimeout_ms;
}

void UART_Read(uint8_t aIndex, void* aOut, uint8_t aOutSize_byte)
{
    // assert(QSCI_QTY > aIndex);

    volatile PortRegs* lR     = PORT_REGS + aIndex;
    Context          * lThis  = sContexts + aIndex;
    HalfContext      * lThisR = lThis->mContexts + UART_READ;

    Interrupt_Disable(aIndex);
    {
        Start_Z0(lThisR, aOut, aOutSize_byte);

        lThisR->mState      = STATE_RX;
        lThisR->mTimeout_ms = 0;

        lR->mCtrl1 |= 0x4; // RE

        Receive_Z0(lThis);
    }
    Interrupt_Enable(aIndex);
}

uint8_t UART_Status(uint8_t aIndex, uint8_t aOp, uint8_t* aCount)
{
    // assert(QSCI_QTY > aIndex);
    // assert(OP_QTY > aOp);

    uint8_t      lResult = UART_ERROR;
    HalfContext* lThisH = sContexts[aIndex].mContexts + aOp;

    Interrupt_Disable(aIndex);
    {
        *aCount = lThisH->mCount;

        switch (lThisH->mState)
        {
        case STATE_COMPLETED:
            lResult        = UART_SUCCESS;
            lThisH->mState = STATE_IDLE;
            break;

        case STATE_ERROR:
            lThisH->mState = STATE_IDLE;
            break;

        case STATE_RX:
        case STATE_TX:
        case STATE_TX_WAIT:
            lResult = UART_PENDING;
            break;

        // default: assert(False);
        }
    }
    Interrupt_Enable(aIndex);

    return lResult;
}

void UART_Tick(uint8_t aIndex, uint8_t aOp, uint16_t aPeriod_ms)
{
    // assert(QSCI_QTY > aIndex);
    // assert(OP_QTY > aOp);

    HalfContext* lThisH = sContexts[aIndex].mContexts + aOp;

    switch (lThisH->mState)
    {
    case STATE_COMPLETED:
    case STATE_ERROR:
    case STATE_IDLE:
        break;

    case STATE_RX:
    case STATE_TX:
    case STATE_TX_WAIT:
        if (0 < lThisH->mTimeout_ms)
        {
            if (lThisH->mTimeout_ms <= aPeriod_ms)
            {
                lThisH->mState      = STATE_ERROR;
                lThisH->mTimeout_ms = 0;
            }
            else
            {
                lThisH->mTimeout_ms -= aPeriod_ms;
            }
        }
        break;

    // default: assert(false);
    }
}

void UART_Write(uint8_t aIndex, const void* aIn, uint8_t aInSize_byte)
{
    // assert(QSCI_QTY > aIndex);

    volatile PortRegs* lR     = PORT_REGS + aIndex;
    Context          * lThis  = sContexts + aIndex;
    HalfContext      * lThisW = lThis->mContexts + UART_WRITE;

    Interrupt_Disable(aIndex);
    {
        Start_Z0(lThisW, (void*) aIn, aInSize_byte);

        lThisW->mState      = STATE_TX;
        lThisW->mTimeout_ms = WRITE_TIMEOUT_ms_byte * aInSize_byte;

        lR->mCtrl1 |= 0x8; // TE

        Send_Z0(lThis);
    }
    Interrupt_Enable(aIndex);
}

// Static fonctions
// //////////////////////////////////////////////////////////////////////////

void IncCount_Z0(HalfContext* aThisH, State aNext, uint16_t aTimeout_ms)
{
    // assert(STATE_QTY > aNext);

    // assert(NULL != aThisH->mInOut);
    // assert(aThisH->mCount < aThis->mSize_byte);

    aThisH->mCount++;

    if (aThisH->mSize_byte <= aThisH->mCount)
    {
        aThisH->mInOut      = NULL;
        aThisH->mTimeout_ms = aTimeout_ms;
        aThisH->mState      = aNext;
    }
}

void Interrupt_Disable(uint8_t aIndex)
{
    // assert(QSCI_QTY > aIndex);

    volatile PortRegs* lR = PORT_REGS + aIndex;

    lR->mCtrl1 &= (~ (CTRL1_REIE | CTRL1_RFIE | CTRL1_TEIE | CTRL1_TIIE));
}

void Interrupt_Enable (uint8_t aIndex)
{
    // assert(QSCI_QTY > aIndex);

    volatile PortRegs* lR    = PORT_REGS + aIndex;
    Context          * lThis = sContexts + aIndex;

    lR->mCtrl1 |= lThis->mEnabledInterrupts;
}

void Interrupt_RCV_Z0(Context* aThis)
{
    // assert(QSCI_QTY > aThis->mIndex);

    volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

    uint16_t lDummy = lR->mStatus;

    Receive_Z0(aThis);
}

void Interrupt_RERR_Z0(Context* aThis)
{
    // assert(QSCI_QTY > aThis->mIndex);

    volatile PortRegs* lR     = PORT_REGS + aThis->mIndex;
    HalfContext      * lThisR = aThis->mContexts + UART_READ;

    uint16_t lStatus = lR->mStatus;

    if (0 != (lStatus & 0x0f00)) // OR NF FE PF
    {
        lThisR->mState = STATE_ERROR;
    }

    if (0 != (lStatus & 0x0f00))
    {
        lStatus &= 0xf0ff;

        lR->mStatus = lStatus;
    }
}

void Interrupt_TIDLE_Z0(Context* aThis)
{
    // assert(QSCI_QTY > aThis->mIndex);

    HalfContext* lThisW = aThis->mContexts + UART_WRITE;

    lThisW->mState = STATE_COMPLETED;

    aThis->mEnabledInterrupts &= ~ CTRL1_TIIE;

    Interrupt_Enable(aThis->mIndex);
}

void Interrupt_TDRE_Z0(Context* aThis)
{
    // assert(QSCI_QTY > aThis->mIndex);

    volatile PortRegs* lR = PORT_REGS + aThis->mIndex;

    uint16_t lDummy = lR->mStatus;

    Send_Z0(aThis);

    Interrupt_Enable(aThis->mIndex);
}

void Receive_Z0(Context* aThis)
{
    // assert(QSCI_QTY > aThis->mIndex);

    volatile PortRegs* lR     = PORT_REGS + aThis->mIndex;
    HalfContext      * lThisR = aThis->mContexts + UART_READ;

    for (;;)
    {
        uint16_t lCtrl2 = lR->mCtrl2;

        unsigned int i;

        // Register value to the RFCNT value
        lCtrl2 >>= 8;
        lCtrl2 &= 0x7;

        if (0 == lCtrl2)
        {
            break;
        }

        for (i = 0; i < lCtrl2; i++)
        {
            uint8_t lData = lR->mData;

            if (NULL == lThisR->mInOut)
            {
                lThisR->mState = STATE_ERROR;
            }
            else
            {
                lThisR->mInOut[lThisR->mCount] = lData;

                IncCount_Z0(lThisR, STATE_COMPLETED, 0);
            }
        }
    }
}

void Send_Z0(Context* aThis)
{
    // assert(QSCI_QTY > aThis->mIndex);

    volatile PortRegs* lR     = PORT_REGS + aThis->mIndex;
    HalfContext      * lThisW = aThis->mContexts + UART_WRITE;

    uint16_t lCtrl2 = lR->mCtrl2;

    unsigned int i;

    // Register value to the TFCNT value
    lCtrl2 >>= 13;
    lCtrl2 &= 0x7;

    // Number of used to number of available
    lCtrl2 = 4 - lCtrl2;

    for (i = 0; i < lCtrl2; i++)
    {
        lR->mData = lThisW->mInOut[lThisW->mCount];

        IncCount_Z0(lThisW, STATE_TX_WAIT, 50);
    }

    aThis->mEnabledInterrupts &= ~ (CTRL1_TEIE | CTRL1_TIIE);

    switch (lThisW->mState)
    {
    case STATE_TX     : aThis->mEnabledInterrupts |= CTRL1_TEIE; break;
    case STATE_TX_WAIT: aThis->mEnabledInterrupts |= CTRL1_TIIE; break;

    // default: assert(false);
    }
}

void Start_Z0(HalfContext* aThisH, void* aInOut, uint8_t aSize_byte)
{
    // assert(NULL != aInOut);
    // assert(0 < aSize_byte);

    aThisH->mCount     = 0;
    aThisH->mInOut     = aInOut;
    aThisH->mSize_byte = aSize_byte;
}
