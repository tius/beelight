//  firmware settings
//
//  see LICENSE file for terms

#pragma once
#include "lite/core/macros.h"
#include XINCLUDE(boards/,BOARD,.h)

//==============================================================================
//  app identity strings
//
//  see build_info.h for build-time version string generation
//------------------------------------------------------------------------------
#define APP_NAME                beelight
#define APP_VERSION             26.05

#define APP_NAME_STR            XSTR(APP_NAME)
#define APP_VERSION_STR         XSTR(APP_VERSION)
#define APP_BANNER_TEXT         APP_NAME_STR " " APP_VERSION_STR

//==============================================================================
//   global log settings
//------------------------------------------------------------------------------
#define LOG_ANSI_COLOR          true        // enable ansi colors
#define LOG_TIMESTAMP           false       // enable log timestamps
#define LOG_LEVEL_PREFIX        false       // enable log level prefix

//==============================================================================
//   event logger settings
//------------------------------------------------------------------------------
#define EVENT_LOG               debug       // enable event logging

//==============================================================================
//  neo pixel settings
//------------------------------------------------------------------------------
#define NEO_GPIO                2           // pin number for neopixel data
#define NEO_PIXEL_CNT           18          // number of neopixels in the strip
#define NEO_COLOR_ORDER         grb         // color order of the neopixels

//==============================================================================
//  infrared receiver settings
//------------------------------------------------------------------------------
#define IR_RX_LOG               none        // log level for ir receiver events
#define IR_RX_GPIO              4           // pin number for infrared receiver data

//==============================================================================
//  infrared transmitter settings
//------------------------------------------------------------------------------
#define IR_TX_LOG               none        // log level for ir transmitter events
#define IR_TX_GPIO              5           // pin number for infrared transmitter data

//==============================================================================
//  I2C settings
//------------------------------------------------------------------------------
#define I2C_SDA_GPIO            0           // pin number for I2C SDA
#define I2C_SCL_GPIO            13          // pin number for I2C SCL
#define I2C_CLOCK_HZ            400000      // I2C clock speed in hertz

//==============================================================================
//  light sensor settings
//------------------------------------------------------------------------------
#define LIGHT_SENSOR_LOG        none        // log level for light sensor events
#define LIGHT_SENSOR_TYPE       VEML3328    // type of light sensor in the system

//==============================================================================
//  accelerometer settings
//------------------------------------------------------------------------------
#define ACC_METER_LOG           none        // log level for accelerometer events

//==============================================================================
//  infrared settings
//------------------------------------------------------------------------------
#define INFRARED_LOG            debug       // log level for infrared events
#define INFRARED_TX_DRIVER      BIT_BANG    // driver for infrared transmitter
//==============================================================================
//  tcs34725 driver settings
//------------------------------------------------------------------------------
#define TCS34725_LOG            none        // log level for tcs34725 events

//==============================================================================
//  veml3328 driver settings
//------------------------------------------------------------------------------
#define VEML3328_LOG            none        // log level for veml3328 events

//==============================================================================
//  bma253 driver settings
//------------------------------------------------------------------------------
#define BMA253_LOG              none        // log level for bma253 events
