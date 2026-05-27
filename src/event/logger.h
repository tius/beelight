//  log events for debugging and testing
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"
#include "event/event.h"

#include "lite/io/log.h"
#include "lite/core/bits.h"

#define LOG_TAG		event
#define LOG_LEVEL	EVENT_LOG

namespace event {

//=============================================================================
class Logger final {
//-----------------------------------------------------------------------------
public:
    Logger(Bus& bus) noexcept
    : hook_(bus, METHOD_THIS(on_event)) {}

//-----------------------------------------------------------------------------
private:

    Hook hook_;

    // log all events as they are published
    void on_event(const Event& event) {
        char buffer[128];
        LOG_DEBUG("%s", event.fmt(buffer));
    }
};

} // namespace event

//=============================================================================
#undef LOG_TAG
#undef LOG_LEVEL
