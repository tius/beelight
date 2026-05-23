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
struct AppEventId : public lite::fsm::EventId {
    enum : u8 {
        IR_RX           = lite::fsm::EventId::COUNT_,     
        LIGHT_LUM,
        LIGHT_RGB,                                          
        ACC_DATA,
    };

    using EventId::EventId; // inherit constructors

    const char* str() const {
        switch (id) {
            case IR_RX:                 return "IR_RX";
            case LIGHT_LUM:             return "LIGHT_LUM";
            case LIGHT_RGB:             return "LIGHT_RGB";
            case ACC_DATA:              return "ACC_DATA";
            default:                    return lite::fsm::EventId::str();
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

struct PayloadAccData {
    u8 x;
    u8 y;
    u8 z;
    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        snprintf(buffer, N, "acc_data x=%u y=%u z=%u", x, y, z);
        return buffer;
    }
};

//-----------------------------------------------------------------------------
struct Payload {
    union {
        PayloadIrRx         ir_rx;
        PayloadLightLum     light_lum;
        PayloadLightRgb     light_rgb;
        PayloadAccData      acc_data;
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
            case Id::LIGHT_LUM:       return p1.light_lum.fmt(buffer);
            case Id::LIGHT_RGB:       return p1.light_rgb.fmt(buffer);
        }
        snprintf(buffer, N, "event %s", id.str()); 
        return buffer;        
    }
};

static_assert(sizeof(AppEvent) == 4, "unexpected size of AppEvent");

//=============================================================================
#pragma pack(pop)
