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

    static u8 p1_byte3_(u32 value) {
        return static_cast<u8>((value >> 24u) & 0xffu);
    }

    static u8 p1_byte2_(u32 value) {
        return static_cast<u8>((value >> 16u) & 0xffu);
    }

    static u8 p1_byte1_(u32 value) {
        return static_cast<u8>((value >> 8u) & 0xffu);
    }

    static u8 p1_byte0_(u32 value) {
        return static_cast<u8>(value & 0xffu);
    }

    // log all events as they are published
    void on_event(const AppEvent& event) {
        if (event.id == AppEventId::LIGHT_STATE) {
            const char* available = (event.p1 != 0u) ? "yes" : "no";
            LOG_INFO(
                "%s available=%s",
                AppEventId(event.id).str(),
                available
            );
            return;
        }

        if (event.id == AppEventId::LIGHT_DATA) {
            LOG_INFO(
                "%s c=%u r=%u g=%u b=%u",
                AppEventId(event.id).str(),
                static_cast<unsigned>(p1_byte3_(event.p1)),
                static_cast<unsigned>(p1_byte2_(event.p1)),
                static_cast<unsigned>(p1_byte1_(event.p1)),
                static_cast<unsigned>(p1_byte0_(event.p1))
            );
            return;
        }

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
