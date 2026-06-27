//  runtime selection and dispatch
//
//  see LICENSE for terms

#pragma once

#include <variant>

#include "settings.h"
#include "app_run.h"
#include "hotspot_run.h"
#include "boot/request.h"

#include "lite/sys/uart.h"
#include "lite/io/serial_out.h"
#include "lite/io/log.h"

//=============================================================================
class RunHost final {
//-----------------------------------------------------------------------------    
public:
    static RunHost& instance() noexcept {
        static RunHost host;
        return host;
    }

    void setup() {
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
    struct NoRun final {
        void loop() noexcept {}
    };

    using Run = std::variant<NoRun, AppRun, HotspotRun>;

    using AppLogger = lite::CustomLogger<
        LOG_ANSI_COLOR,
        LOG_TIMESTAMP,
        LOG_LEVEL_PREFIX
    >;

    lite::Uart          uart_       {MONITOR_SPEED};
    lite::SerialOut     serial_out_ {uart_, "\n-----\n"};
    lite::StdOut        std_out_    {serial_out_};
    AppLogger           logger_     {serial_out_};

    Run run_ {};

    RunHost() = default;

    RunHost(const RunHost&) = delete;
    RunHost& operator=(const RunHost&) = delete;
};