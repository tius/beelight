//  hotspot runtime
//
//  see LICENSE for terms

#pragma once

#include "runtime_core.h"

//=============================================================================
class HotspotRun final {
//-----------------------------------------------------------------------------
public:
    HotspotRun() {
        core_.ready();
        core_.status().set(RgbState::HOTSPOT);
    }

    void loop() {
        core_.loop();
    }

//-----------------------------------------------------------------------------
private:
    RuntimeCore core_ {};
};
