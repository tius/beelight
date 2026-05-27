//  runtime selection and dispatch
//
//  see LICENSE for terms

#pragma once

#include <variant>

#include "app_run.h"
#include "hotspot_run.h"
#include "boot/request.h"

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
            case boot::Mode::app:
                run_.emplace<AppRun>();
                return;

            case boot::Mode::hotspot:
                run_.emplace<HotspotRun>();
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

    Run run_ {};

    RunHost() = default;

    RunHost(const RunHost&) = delete;
    RunHost& operator=(const RunHost&) = delete;
};