//  boot button hold probe
//
//  blocks from reset until button release or ceiling,
//  drives back led for visual feedback during recovery hold
//
//  see LICENSE for terms

#pragma once

#include "settings.h"
#include "back_led/back_led.h"

#include "lite/sys/gpio.h"
#include "lite/sys/clock.h"
#include "lite/io/log.h"
#include "lite/core/compile_time.h"
#include "lite/core/changed.h"

#define LOG_TAG     button_probe
#define LOG_LEVEL   BUTTON_PROBE_LOG

//==============================================================================
class ButtonProbe final {

using u8  = lite::u8;
using u32 = lite::u32;

using ButtonGpio = lite::gpio::Input<BUTTON_GPIO>;

// jewel tone palette — one color per second, cycles if held longer
static constexpr lite::Rgb8 k_colors[] = {
    {  72,   8,  16 },  // red
    {  80,  40,   0 },  // orange
    {  68,  66,   4 },  // yellow
    {   4,  64,  28 },  // green
    {   4,  24,  80 },  // blue
    {  48,   8,  72 },  // violet
};

//------------------------------------------------------------------------------
public:
    u32 wait_release(u32 max_wait_ms) {
        ButtonGpio button;
        BackLed    led;

        if (button.is_hi()) {
            LOG_TRACE("no hold");
            return 0;
        }

        const u32 start_ms = lite::now_ms();
        u8 last_sec = 0xff;

        while (true) {
            const u32 hold_ms = lite::now_ms() - start_ms;
            const u8  sec     = static_cast<u8>(hold_ms / 1000);

            // advance led color each second
            if (lite::changed(last_sec, sec)) {
                led.set(k_colors[sec % lite::array_count(k_colors)]);
            }

            if (button.is_hi()) {
                LOG_TRACE("released: %lu ms", static_cast<unsigned long>(hold_ms));
                return hold_ms;
            }
            if (hold_ms >= max_wait_ms) {
                LOG_TRACE("max_wait: %lu ms", static_cast<unsigned long>(hold_ms));
                return hold_ms;
            }

            delay(1);
        }
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
