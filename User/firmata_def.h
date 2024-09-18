// firmata_def.h

#pragma once

#define PIN_DIG_INPUT 1
#define PIN_DIG_OUTPUT 2
#define PIN_ANALOG_INPUT 4
#define PIN_PWM 8
#define PIN_DIG_PULLUP 16
// частный случай PWM
#define PIN_SERVO 32

// sysex commands
#define _START_SYSEX 0xF0
#define _END_SYSEX 0xF7

#define REPORT_FIRMWARE 0x79
#define CAPABILITY_QUERY 0x6B
#define CAPABILITY_RESPONSE 0x6C
#define ANALOG_MAPPING_QUERY 0x69
#define ANALOG_MAPPING_RESPONSE 0x6A

// from FirmataConstants.h
static const uint8_t REPORT_VERSION =          0xF9; // report protocol version
static const uint8_t SYSTEM_RESET =            0xFF; // reset from MIDI
// для отправки нужны не макросы, а константы
static const uint8_t START_SYSEX =             0xF0; // start a MIDI Sysex message
static const uint8_t END_SYSEX =               0xF7; // end a MIDI Sysex message

// from FirmateDefines.h
#define FIRMATA_PROTOCOL_MAJOR_VERSION  2 // for non-compatible changes
#define FIRMATA_PROTOCOL_MINOR_VERSION  5 // for backwards compatible changes
