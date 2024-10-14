
// Author    KMS - Martin Dubois, P. Eng.
// Copyright (C) 2024 KMS
// License   http://www.apache.org/licenses/LICENSE-2.0
// Product   KMS-uC
// File      Includes/Modbus.h

#pragma once

// Reference
// //////////////////////////////////////////////////////////////////////////
//
// MODBUS APPLICATION PROTOCOL SPECIFICATION V1.1b3
// https://modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf

// Constants
// //////////////////////////////////////////////////////////////////////////

#define MODBUS_BYTE_DEVICE    (0)
#define MODBUS_BYTE_FUNCTION  (1)
#define MODBUS_BYTE_EXCEPTION (2)

#define MODBUS_EXCEPTION_ILLEGAL_FUNCTION                        (0x01)
#define MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS                    (0x02)
#define MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE                      (0x03)
#define MODBUS_EXCEPTION_SERVER_DEVICE_FAILURE                   (0x04)
#define MODBUS_EXCEPTION_ACKNOWLEDGE                             (0x05)
#define MODBUS_EXCEPTION_SERVER_DEVICE_BUSY                      (0x06)
#define MODBUS_EXCEPTION_MEMORY_PARITY_ERROR                     (0x08)
#define MODBUS_EXCEPTION_GATEWAY_PATH_UNAVAILABLE                (0x0a)
#define MODBUS_EXCEPTION_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND (0x0b)

#define MODBUS_FUNCTION_READ_HOLDING_REGISTERS   (0x03)
#define MODBUS_FUNCTION_READ_INPUT_REGISTERS     (0x04)
#define MODBUS_FUNCTION_WRITE_SINGLE_REGISTER    (0x06)
#define MODBUS_FUNCTION_WRITE_MULTIPLE_REGISTERS (0x10)

#define MODBUS_FUNCTION_ERROR (0x80)

#define MODBUS_NO_ERROR (0)
