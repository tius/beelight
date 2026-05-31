//  manage button-triggered shutdown
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"
#include "power/power.h"

#include "lite/cli/cmd.h"
#include "lite/core/method_bind.h"
#include "lite/core/timer.h"
#include "lite/sys/gpio.h"

//=============================================================================
class ButtonShutdown final {
//-----------------------------------------------------------------------------
public:
    ButtonShutdown(lite::CmdShell& shell, Power& power)
        : power_(power)
        , cmd_off_(shell, "off", "shutdown system", "", METHOD_THIS(on_cmd_off))
    {
        timer_.start(lite::duration_ms{BUTTON_SHUTDOWN_HOLD_MS});
    }

//-----------------------------------------------------------------------------
private:
    using ButtonGpio = lite::gpio::Input<BUTTON_GPIO>;

    Power&          power_;
    ButtonGpio      button_gpio_;
    lite::Timer     timer_      {MSG_THIS(on_timer)};
    lite::Cmd       cmd_off_;

    [[nodiscard]] bool button_is_down() const noexcept {
        return button_gpio_.is_lo();
    }

    void on_timer() {
        if ( button_is_down() ) {
            shutdown();
        }
    }

    void on_cmd_off(lite::Out&, lite::Args) {
        shutdown();
    }

    void shutdown() {
        power_.shutdown();
    }
};

//=============================================================================