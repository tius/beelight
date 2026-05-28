//  shared runtime services
//
//  see LICENSE for terms

#pragma once

#include "settings.h"
#include "event/event.h"
#include "event/logger.h"
#include "status_led/rgb_show.h"

#include "lite/cli/cmd.h"
#include "lite/cli/console.h"
#include "lite/io/serial_out.h"
#include "lite/io/log.h"
#include "lite/sys/clock.h"
#include "lite/core/timer.h"
#include "lite/cmd/sys_cmd.h"

//==============================================================================
class RuntimeCore final {
//------------------------------------------------------------------------------
public:
    RuntimeCore() = default;

    RuntimeCore(const RuntimeCore&) = delete;
    RuntimeCore& operator=(const RuntimeCore&) = delete;

    void ready() {
        lite::std_out->println(APP_BANNER_TEXT);
        console_.ready();
    }

    void loop() {
        lite::Timer::spin(lite::now());

        if (auto c = uart_.rx(); c >= 0) {
            console_.process(char(c));
        }
    }

    [[nodiscard]] event::Bus& event_bus() noexcept {
        return event_bus_;
    }

    [[nodiscard]] lite::CmdShell& shell() noexcept {
        return shell_;
    }

    [[nodiscard]] RgbShow& status() noexcept {
        return status_;
    }

//------------------------------------------------------------------------------
private:
    using AppLogger = lite::CustomLogger<
        LOG_ANSI_COLOR,
        LOG_TIMESTAMP,
        LOG_LEVEL_PREFIX
    >;

    event::Bus         event_bus_  {};

    lite::Uart         uart_       {MONITOR_SPEED};
    lite::SerialOut    serial_out_ {uart_, "\n-----\n"};
    lite::StdOut       std_out_    {serial_out_};
    AppLogger          logger_     {serial_out_};

    lite::CmdShell     shell_      {};
    lite::Console      console_    {shell_, serial_out_};
    lite::cmd::SysCmd  cmd_sys_    {shell_};

    event::Logger      event_logger_{event_bus_};

    RgbLed             rgb_led_    {};
    RgbShow            status_     {rgb_led_};
    lite::Cmd          cmd_led_    {shell_, "led", "set rgb status state", "<state>", METHOD_THIS(on_cmd_led)};

    void on_cmd_led(lite::Out& out, lite::Args args) {
        (void)out;
        status_.set(args.get_u16());
    }
};
