//  firmware settings
//
//  see LICENSE file for terms

#pragma once

//==============================================================================
//  app identity strings
//
//  see build_info.h for build-time version string generation
//------------------------------------------------------------------------------
#define APP_NAME            beelight

#define APP_NAME_STR        XSTR(APP_NAME)
#define APP_VERSION_STR     XSTR(APP_VERSION)
#define APP_BANNER_TEXT     APP_NAME_STR " " APP_VERSION_STR

//==============================================================================
//   log settings
//------------------------------------------------------------------------------
#define LOG_TIMESTAMP               true        // enable log timestamps
#define LOG_ANSI_COLOR              true        // enable ansi colors
