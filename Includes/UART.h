
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/UART.h

#pragma once

// Constants
// //////////////////////////////////////////////////////////////////////////

#define UART_READ  (0)
#define UART_WRITE (1)

#define UART_ERROR   (0)
#define UART_PENDING (1)
#define UART_SUCCESS (2)

// Functions
// //////////////////////////////////////////////////////////////////////////

extern void UART_Init(uint8_t aIndex);

// aOp  UART_READ
//      UART_WRITE
extern void UART_Abort(uint8_t aIndex, uint8_t aOp);

// aOp  UART_READ
//      UART_WRITE
//
// Return  0  Not idle
//         1  Idle
extern uint8_t UART_Idle(uint8_t aIndex, uint8_t aOp);

// aOp  UART_READ
//      UART_WRITE
extern void UART_SetTimeout(uint8_t aIndex, uint8_t aOp, uint16_t aTimeout_ms);

extern void UART_Read(uint8_t aIndex, void* aOut, uint8_t aOutSize_byte);

// Return  UART_ERROR
//         UART_PENDING
//         UART_SUCCESS
extern uint8_t UART_Status(uint8_t aIndex, uint8_t aOp, uint8_t* aCount);

extern void UART_Tick(uint8_t aIndex, uint8_t aOp, uint16_t aPeriod_ms);

extern void UART_Write(uint8_t aIndex, const void* aIn, uint8_t aInSize_byte);
