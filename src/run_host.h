//  runtime selection and dispatch
//
//  see LICENSE for terms

#pragma once

#include <variant>

#include "settings.h"
#include "app_run.h"
#include "hotspot_run.h"
#include "boot/request.h"
#include "boot/button_probe.h"
#include "boot/uptime_probe.h"

#include "lite/sys/uart.h"
#include "lite/io/serial_out.h"
#include "lite/io/log.h"

#define LOG_TAG     run_host
#define LOG_LEVEL   RUNTIME_LOG

//=============================================================================
class RunHost final {
//-----------------------------------------------------------------------------    
public:
    static RunHost& instance() noexcept {
        static RunHost host;
        return host;
    }

    void setup() {
        ButtonProbe button;
        auto hold_ms = button.wait_release(BUTTON_PROBE_MAX_WAIT);
        auto prev_uptime = uptime_.track();
        LOG_INFO("boot hold: %lu ms, prev uptime: %u ms",
            static_cast<unsigned long>(hold_ms),
            static_cast<unsigned>(prev_uptime));

        auto mode = boot::startup_mode();

        switch (mode) {
            case boot::Mode::hotspot:
                run_.emplace<HotspotRun>(uart_, serial_out_);
                return;

            default:
                run_.emplace<AppRun>(uart_, serial_out_);
                return;
        }
    }

    void loop() {
        std::visit( 
            [](auto& run) {
                run.loop();
            },
            run_
        );
    }

//-----------------------------------------------------------------------------    
private:
    //  helper class to provide a no-op run variant for std::variant
    struct NoRun final {
        void loop() noexcept {}
    };

    //  runtime selection variant
    using Run = std::variant<NoRun, AppRun, HotspotRun>;

    //  initialize logging and serial output before any runtime is constructed
    using AppLogger = lite::CustomLogger<
        LOG_ANSI_COLOR,
        LOG_TIMESTAMP,
        LOG_LEVEL_PREFIX
    >;

    lite::Uart          uart_       {MONITOR_SPEED};
    lite::SerialOut     serial_out_ {uart_, "\n-----\n"};
    lite::StdOut        std_out_    {serial_out_};
    AppLogger           logger_     {serial_out_};

    UptimeProbe         uptime_;

    //  runtime selection and dispatch
    Run run_ {};

    //  prevent copying
    RunHost() = default;
    RunHost(const RunHost&) = delete;
    RunHost& operator=(const RunHost&) = delete;
};

#undef LOG_TAG
#undef LOG_LEVEL