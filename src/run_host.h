//  runtime selection and dispatch
//
//  see LICENSE for terms

#pragma once

#include <variant>

#include "app_run.h"
#include "boot_mode.h"

//=============================================================================
class RunHost final {
//-----------------------------------------------------------------------------    
public:
    static RunHost& instance() noexcept {
        static RunHost host;
        return host;
    }

    void setup() {
        auto mode = select_boot_mode();

        switch (mode) {
            case BootMode::app:
                run_.emplace<AppRun>();
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

    using Run = std::variant<NoRun, AppRun>;

    Run run_ {};

    RunHost() = default;

    RunHost(const RunHost&) = delete;
    RunHost& operator=(const RunHost&) = delete;

    static constexpr BootMode select_boot_mode() noexcept {
        return BootMode::app;
    }
};