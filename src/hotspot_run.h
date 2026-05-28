//  hotspot runtime
//
//  see LICENSE for terms

#pragma once

#include "back_led/back_state.h"
#include "runtime_core.h"

//=============================================================================
class HotspotRun final {
//-----------------------------------------------------------------------------
public:
    HotspotRun() {
        core_.ready();
        core_.back_show().set(BackState::HOTSPOT);
    }

    void loop() {
        core_.loop();
    }

//-----------------------------------------------------------------------------
private:
    RuntimeCore core_ {};
};
