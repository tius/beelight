//  log events for debugging and testing
//
//  see LiCENSE file for terms

#pragma once

#include "lite/io/log.h"
#include "lite/core/event_bus.h"
#include "lite/core/bits.h"
#include "app_event.h"

#define LOG_TAG		event
#define LOG_LEVEL	trace

//=============================================================================
class EventLogger final {
public:
    EventLogger(lite::EventBus<AppEvent>& event_bus) noexcept
    : event_hook_(event_bus, METHOD_THIS(on_event)) {}

private:
    lite::EventHook<AppEvent> event_hook_;

    // log all events as they are published
    void on_event(const AppEvent& event) {
        LOG_INFO(
            "%s p1=%04x %04x", 
            AppEventId(event.id).str(), 
            lite::hi_word(event.p1), 
            lite::lo_word(event.p1)
    );
    }
};

//=============================================================================
#undef LOG_TAG
#undef LOG_LEVEL
