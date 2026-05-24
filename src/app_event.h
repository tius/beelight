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
using lite::s16;

#pragma pack(push, 1)

//=============================================================================
struct AppEventId : public lite::fsm::EventId {
    enum : u8 {
        IR_RX           = lite::fsm::EventId::COUNT_,
        PWM_SUSPEND,
        PWM_RESUME,
        LIGHT_LUM,
        LIGHT_RGB,
        TEMP,
        TILT,
    };

    using EventId::EventId; // inherit constructors

    const char* str() const {
        switch (id) {
            case IR_RX:                 return "IR_RX";
            case PWM_SUSPEND:           return "PWM_SUSPEND";
            case PWM_RESUME:            return "PWM_RESUME";
            case LIGHT_LUM:             return "LIGHT_LUM";
            case LIGHT_RGB:             return "LIGHT_RGB";
            case TEMP:                  return "TEMP";
            case TILT:                  return "TILT";
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

struct PayloadTemp {
    s16 celsius10;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        snprintf(buffer, N, "temp %d.%d°C", int(celsius10) / 10, int(celsius10) % 10);
        return buffer;
    }
};

struct PayloadTilt {
    u8 pitch;
    u8 roll;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        snprintf(buffer, N, "tilt pitch=%u roll=%u", pitch, roll);
        return buffer;
    }
};

//-----------------------------------------------------------------------------
struct Payload {
    union {
        PayloadIrRx         ir_rx;
        PayloadLightLum     light_lum;
        PayloadLightRgb     light_rgb;
        PayloadTilt      tilt;
        PayloadTemp         temp;
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
            case Id::IR_RX:             return p1.ir_rx.fmt(buffer);
            case Id::PWM_SUSPEND:
                snprintf(buffer, N, "pwm_suspend");
                return buffer;
            case Id::PWM_RESUME:
                snprintf(buffer, N, "pwm_resume");
                return buffer;
            case Id::LIGHT_LUM:         return p1.light_lum.fmt(buffer);
            case Id::LIGHT_RGB:         return p1.light_rgb.fmt(buffer);
            case Id::TILT:              return p1.tilt.fmt(buffer);
            case Id::TEMP:              return p1.temp.fmt(buffer);
        }
        snprintf(buffer, N, "event %s", id.str()); 
        return buffer;        
    }
};

static_assert(sizeof(AppEvent) == 4, "unexpected size of AppEvent");

//=============================================================================
#pragma pack(pop)
