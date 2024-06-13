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
#define START_SYSEX 0xF0
#define END_SYSEX 0xF7

#define REPORT_FIRMWARE 0x79
#define CAPABILITY_QUERY 0x6B
#define CAPABILITY_RESPONSE 0x6C
