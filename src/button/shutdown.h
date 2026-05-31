//  manage button-triggered battery_off
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"
#include "power/battery.h"
#include "power/power.h"

#include "lite/cli/cmd.h"
#include "lite/core/method_bind.h"
#include "lite/core/timer.h"
#include "lite/sys/gpio.h"

//=============================================================================
class ButtonShutdown final {
//-----------------------------------------------------------------------------
public:
    ButtonShutdown(lite::CmdShell& shell, Power& power, Battery& battery)
        : power_(power)
        , battery_(battery)
        , cmd_off_(shell, "off", "turn battery off", "", METHOD_THIS(on_cmd_off))
    {
        timer_.start(lite::duration_ms{BUTTON_SHUTDOWN_HOLD_MS});
    }

//-----------------------------------------------------------------------------
private:
    using ButtonGpio = lite::gpio::Input<BUTTON_GPIO>;

    Power&          power_;
    Battery&        battery_;
    ButtonGpio      button_gpio_;
    lite::Timer     timer_      {MSG_THIS(on_timer)};
    lite::Cmd       cmd_off_;

    [[nodiscard]] bool button_is_down() const noexcept {
        return button_gpio_.is_lo();
    }

    void on_timer() {
        if ( button_is_down() ) {
            battery_.shutdown();
        }
    }

    void on_cmd_off(lite::Out&, lite::Args) {
        battery_off();
    }

    void battery_off() {
        battery_.shutdown();
        power_.battery_off();
    }
};

//=============================================================================