
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/ADC.h

#pragma once

// Constants
// //////////////////////////////////////////////////////////////////////////

#define ADC_ERROR         (0x01)
#define ADC_HIGH_LIMIT    (0x02)
#define ADC_LOW_LIMIT     (0x04)
#define ADC_NOT_READY     (0x08)
#define ADC_ZERO_CROSSING (0x10)

#define ADC_INTERRUPT_END_OF_SCAN   (0x01)
#define ADC_INTERRUPT_HIGH_LIMIT    (0x02)
#define ADC_INTERRUPT_LOW_LIMIT     (0x04)
#define ADC_INTERRUPT_ZERO_CROSSING (0x08)

#define ADC_SIGNED (0x80)

// Functions
// //////////////////////////////////////////////////////////////////////////

// aChannel     Each value give the input index. User ADC_SIGNED | Index it the
//              channel produce signed value.
// aChannelQty
// aInterrupts  ACD_INTERRUPT_END_OF_SCAN, ADC_INTERRUPT_HIGH_LIMIT,
//              ADC_INTERRUPT_LOW_LIMIT or/and ADC_INTERRUPT_ZERO_CROSSING
extern void ADC_Init(const uint8_t* aChannels, uint8_t aChannelQty, uint8_t aInterrupts);

// Return  ADC_ERROR, ADC_HIGH_LIMIT, ADC_LOW_LIMIT, ADC_NOT_READY or/and
//         ADC_ZERO_CROSSING
extern uint8_t ADC_GetValue_Signed(uint8_t aChannel, int16_t* aOut);

extern uint8_t ADC_GetValue_Unsigned(uint8_t aChannel, uint16_t* aOut);

extern void ADC_SetLimits(uint8_t aChannel, uint16_t aLow, uint16_t aHigh);

extern void ADC_AcknowledgeInterrupt();
