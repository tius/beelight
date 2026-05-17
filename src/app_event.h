//  application event definitions
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/types.h"
#include "lite/core/fsm.h"
#include "lite/core/event_bus.h"

class AppEvent;

using lite::u8;
using lite::u16;
using lite::u32;

#pragma pack(push, 1)

//=============================================================================
struct AppEventId {
    enum : u8 {
        NONE,
        ENTER 			= lite::fsm::event::ENTER,
        LEAVE 			= lite::fsm::event::LEAVE,
        TIMEOUT 		= lite::fsm::event::TIMEOUT,

        IR_RX           = lite::fsm::event::COUNT_,     
        LIGHT_STATE,
        LIGHT_LUM,
        LIGHT_RGB,                                          
    };

    u8 id = 0;

    constexpr AppEventId() = default;
    constexpr AppEventId(u8 v) : id(v) {}
    constexpr operator u8() const { return id; }

    const char* str() const {
        switch (id) {
            case NONE:                  return "NONE";
            case ENTER:                 return "ENTER";
            case LEAVE:                 return "LEAVE";
            case TIMEOUT:               return "TIMEOUT";
            case IR_RX:                 return "IR_RX";
            case LIGHT_STATE:           return "LIGHT_STATE";
            case LIGHT_LUM:             return "LIGHT_LUM";
            case LIGHT_RGB:             return "LIGHT_RGB";
            default:                    return "?";
        }
    }
};

static_assert(sizeof(AppEventId) == 1, "unexpected size of AppEventId");

//=============================================================================
struct PayloadIrRx {
    u8      addr;
    u8      cmd;
    bool    repeat;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        snprintf(buffer, N, "ir_rx %02X.%02X %s", addr, cmd, repeat ? "r" : "");
        return buffer;
    }
};    

struct PayloadLightState {
    bool available;
    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        snprintf(buffer, N, "light_state %s", available ? "available" : "non-available");
        return buffer;
    }
};

struct PayloadLightLum {
    u8      y;
    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        snprintf(buffer, N, "light_lum %u", y);
        return buffer;
    }
};

struct PayloadLightRgb {
    u8      r;
    u8      g;
    u8      b;
    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        snprintf(buffer, N, "light_rgb #%02x%02x%02x", r, g, b);
        return buffer;
    }
};

//-----------------------------------------------------------------------------
struct Payload {
    union {
        PayloadIrRx         ir_rx;
        PayloadLightState   light_state;
        PayloadLightLum     light_lum;
        PayloadLightRgb     light_rgb;
    };
};

static_assert(sizeof(Payload) == 3, "unexpected size of Payload");

//=============================================================================
struct AppEvent {
    using Id = AppEventId;

    Id      id = Id::NONE;
    Payload p1 = {};    

	[[nodiscard]] constexpr explicit operator bool() const noexcept {
		return id != Id::NONE;
	}

    AppEvent() = default;
    AppEvent(int id, Payload p1 = {}) : id(id), p1(p1) {}

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        switch (id) {
            case Id::IR_RX:           return p1.ir_rx.fmt(buffer);
            case Id::LIGHT_STATE:     return p1.light_state.fmt(buffer);
            case Id::LIGHT_LUM:       return p1.light_lum.fmt(buffer);
            case Id::LIGHT_RGB:       return p1.light_rgb.fmt(buffer);
        }
        snprintf(buffer, N, "event id=%u", id.id); 
        return buffer;        
    }
};

static_assert(sizeof(AppEvent) == 4, "unexpected size of AppEvent");

//=============================================================================
#pragma pack(pop)
