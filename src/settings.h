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
//   log settings
//------------------------------------------------------------------------------
#define LOG_ANSI_COLOR          true        // enable ansi colors
#define LOG_TIMESTAMP           false       // enable log timestamps
#define LOG_LEVEL_PREFIX        false       // enable log level prefix

//==============================================================================
//  neo pixel settings
//------------------------------------------------------------------------------
#define NEO_GPIO                2           // pin number for neopixel data
#define NEO_PIXEL_CNT           18           // number of neopixels in the strip
#define NEO_COLOR_ORDER         grb         // color order of the neopixels

//==============================================================================
//  infrared receiver settings
//------------------------------------------------------------------------------
#define IR_RX_GPIO              4           // pin number for infrared receiver data
#define IR_RX_ACTIVE_HIGH       true        // set true for inverted receiver output
