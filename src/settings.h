//  firmware settings
//
//  see LICENSE file for terms

#pragma once
#include "build_info.h"

#include "lite/core/macros.h"
#include XINCLUDE(boards/,BOARD,.h)

//==============================================================================
//  app identity strings
//
//  see build_info.h for build-time version string generation
//------------------------------------------------------------------------------
#define APP_NAME                beelight

#define APP_NAME_STR            XSTR(APP_NAME)
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
#define EVENT_LOG               none        // enable event logging
#define EVENT_QUEUE_SIZE        8           // async event fifo capacity

//==============================================================================
//  hotspot settings
//------------------------------------------------------------------------------
#define HOTSPOT_SSID_PREFIX     "bee-"      // prefix before device id
#define HOTSPOT_PSK             "honeypot"  // access point password

#define HOTSPOT_IP              lite::ipv4(192, 168, 4, 1)
#define HOTSPOT_GATEWAY         lite::ipv4(192, 168, 4, 1)
#define HOTSPOT_NETMASK         lite::ipv4(255, 255, 255, 0)
#define HOTSPOT_LEASE_START     lite::ipv4(192, 168, 4, 2)
#define HOTSPOT_LEASE_END       lite::ipv4(192, 168, 4, 5)

//==============================================================================
//  button settings
//------------------------------------------------------------------------------
#define BUTTON_GPIO             16          // pin number for wake button
#define BUTTON_SHUTDOWN_HOLD_MS 3000        // hold duration for shutdown

//==============================================================================
//  wake info settings
//------------------------------------------------------------------------------
#define WAKE_INFO_LOG           none        // log level for wake button events
#define WAKE_INFO_KEEP_ALIVE_MS 1500        // keep rtc uptime fresh after release

//==============================================================================
//  wake morse settings
//------------------------------------------------------------------------------
#define WAKE_MORSE_DOT_MAX_MS   250         // longest dot press duration
#define WAKE_MORSE_CHAR_GAP_MS  500         // gap ending one morse character
#define WAKE_MORSE_DONE_GAP_MS  1200        // gap ending the command

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
#define IR_TX_ACTIVE_LOW        true        // drive low to turn ir led on

//==============================================================================
//  I2C settings
//------------------------------------------------------------------------------
#define I2C_SDA_GPIO            0           // pin number for I2C SDA
#define I2C_SCL_GPIO            13          // pin number for I2C SCL
#define I2C_CLOCK_HZ            400000      // I2C clock speed in hertz

//==============================================================================
//  power settings
//------------------------------------------------------------------------------
#define POWER_LOG               debug       // log level for power events

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
#define INFRARED_LOG            none        // log level for infrared events
#define INFRARED_TX_DRIVER      TIMER1      // driver for infrared transmitter

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

//==============================================================================
//  mp2667 driver settings
//------------------------------------------------------------------------------
#define MP2667_LOG              trace       // log level for mp2667 events
#define MP2667_I2C_ADDR         0x09        // I2C address of mp2667 charger
#define MP2667_INPUT_MIN_MV     4600        // input voltage regulation limit
#define MP2667_INPUT_MA         470         // input current limit
#define MP2667_CHARGE_MA        257         // fast charge current
#define MP2667_TERM_MA          52          // charge termination current
#define MP2667_DISCHARGE_MA     1000        // battery discharge current limit
#define MP2667_BATTERY_REG_MV   4200        // battery regulation voltage
#define MP2667_BATTERY_UVLO_MV  2800        // battery discharge cutoff threshold
#define MP2667_PRECHARGE_MV     3000        // pre-charge to fast-charge threshold
#define MP2667_RECHARGE_MV      300         // recharge threshold below regulation
#define MP2667_WATCHDOG_S       0           // I2C watchdog timeout, 0 disables it
#define MP2667_SAFETY_TIMER_H   12          // fast charge safety timer
#define MP2667_TMR2X_ENABLED    true        // extend safety timer during PPM
#define MP2667_THERMAL_REG_C    100         // thermal regulation threshold
#define MP2667_NTC_ENABLED      false       // no thermistor connected
