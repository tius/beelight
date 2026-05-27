//  log events for debugging and testing
//
//  see LiCENSE file for terms

#pragma once

#include "run_event.h"
#include "lite/io/log.h"
#include "lite/core/event_bus.h"
#include "lite/core/bits.h"

#define LOG_TAG		event
#define LOG_LEVEL	EVENT_LOG

//=============================================================================
class EventLogger final {
//-----------------------------------------------------------------------------
using EventBus = lite::EventBus<RunEvent>;
using EventHook = lite::EventHook<RunEvent>;
//-----------------------------------------------------------------------------
public:
    EventLogger(EventBus& event_bus) noexcept
    : event_hook_(event_bus, METHOD_THIS(on_event)) {}

//-----------------------------------------------------------------------------
private:

    EventHook event_hook_;

    // log all events as they are published
    void on_event(const RunEvent& event) {
        char buffer[128];
        LOG_DEBUG("%s", event.fmt(buffer));
    }
};

//=============================================================================
#undef LOG_TAG
#undef LOG_LEVEL
