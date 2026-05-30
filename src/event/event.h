//  runtime event definitions
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"
#include "driver/ir_code.h"
#include "power/power_state.h"

#include "lite/core/types.h"
#include "lite/core/fsm.h"
#include "lite/core/event_bus.h"
#include "lite/core/event_queue.h"
#include "lite/core/ipv4.h"
#include "lite/core/text_buffer.h"
#include "lite/esp8266/hotspot.h"

namespace event {

struct Event;

using lite::u8;
using lite::u16;
using lite::u32;
using lite::s16;

#pragma pack(push, 1)

//=============================================================================
struct Id : public lite::fsm::EventId {
    enum : u8 {
        MORSE_CMD           = lite::fsm::EventId::COUNT_,
        IR_RX,
        LIGHT_LUM,
        LIGHT_RGB,
        TEMP,
        TILT,
        POWER_STATE,
        HOTSPOT_STARTED,
        HOTSPOT_FAILED,
        HOTSPOT_CLIENT_COUNT,
    };

    using lite::fsm::EventId::EventId;

    const char* str() const {
        switch (id) {
            case MORSE_CMD:             return "MORSE_CMD";
            case IR_RX:                 return "IR_RX";
            case LIGHT_LUM:             return "LIGHT_LUM";
            case LIGHT_RGB:             return "LIGHT_RGB";
            case TEMP:                  return "TEMP";
            case TILT:                  return "TILT";
            case POWER_STATE:           return "POWER_STATE";
            case HOTSPOT_STARTED:       return "HOTSPOT_STARTED";
            case HOTSPOT_FAILED:        return "HOTSPOT_FAILED";
            case HOTSPOT_CLIENT_COUNT:  return "HOTSPOT_CLIENT_COUNT";
            default:                    return lite::fsm::EventId::str();
        }
    }
};

static_assert(sizeof(Id) == 1, "unexpected size of event::Id");

//=============================================================================
struct MorseCmd {
    char text[3];
    u8 len;

    template <size_t N>
    [[nodiscard]] bool is(const char (&value)[N]) const noexcept {
        static_assert(N > 0u);
        constexpr auto value_len = static_cast<u8>(N - 1u);

        if (len != value_len || value_len > sizeof(text)) {
            return false;
        }

        for (u8 idx = 0; idx < value_len; ++idx) {
            if (text[idx] != value[idx]) {
                return false;
            }
        }

        return true;
    }

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text_buffer(buffer);
        text_buffer.appendf(
            "morse_cmd %.*s",
            static_cast<int>(len),
            text
        );
        return buffer;
    }
};

struct IrRx {
    IrCode code;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);

        IrCode::Nec nec;
        if (code.decode_nec(nec)) {
            text.appendf("ir_rx %02X.%02X", nec.addr, nec.cmd);
            return buffer;
        }

        if (code.is_repeat()) {
            text.append("ir_rx repeat");
            return buffer;
        }

        if (code.is_invalid()) {
            text.append("ir_rx invalid");
            return buffer;
        }

        text.appendf("ir_rx %08lX", static_cast<unsigned long>(code.raw()));
        return buffer;
    }
};    

struct LightLum {
    u8      y;
    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("light_lum %u", y);
        return buffer;
    }
};

struct LightRgb {
    u8      r;
    u8      g;
    u8      b;
    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("light_rgb #%02x%02x%02x", r, g, b);
        return buffer;
    }
};

struct Temp {
    s16 celsius10;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("temp %d.%d°C", int(celsius10) / 10, int(celsius10) % 10);
        return buffer;
    }
};

struct Tilt {
    u8 pitch;
    u8 roll;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("tilt pitch=%u roll=%u", pitch, roll);
        return buffer;
    }
};

struct HotspotStarted {
    lite::Ipv4 ip;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        char ip_buf[16];
        lite::TextBuffer text(buffer);
        text.appendf("hotspot_started ip=%s", ip.fmt(ip_buf));
        return buffer;
    }
};

struct HotspotFailed {
    lite::esp8266::HotspotStatus status;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("hotspot_failed %s", status.str());
        return buffer;
    }
};

struct HotspotClientCount {
    u8 count;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("hotspot_client_count %u", count);
        return buffer;
    }
};

template <size_t N>
const char* fmt_power_state(char (&buffer)[N], PowerState state) {
    lite::TextBuffer text(buffer);
    text.appendf("power_state %s", state.str());
    return buffer;
}

//-----------------------------------------------------------------------------
struct Payload {
    union {
        MorseCmd            morse_cmd;
        IrRx                ir_rx;
        LightLum            light_lum;
        LightRgb            light_rgb;
        Tilt                tilt;
        Temp                temp;
        PowerState          power_state;
        HotspotStarted      hotspot_started;
        HotspotFailed       hotspot_failed;
        HotspotClientCount  hotspot_client_count;
    };
};

static_assert(sizeof(Payload) == 4, "unexpected size of Payload");

//=============================================================================
struct Event {
    using Id = event::Id;

    Id      id = Id::NONE;
    Payload p1 = {};    

	[[nodiscard]] constexpr explicit operator bool() const noexcept {
		return id != Id::NONE;
	}

    Event() = default;
    Event(int id, Payload p1 = {}) : id(id), p1(p1) {}

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        switch (id) {
            case Id::MORSE_CMD:         return p1.morse_cmd.fmt(buffer);
            case Id::IR_RX:             return p1.ir_rx.fmt(buffer);
            case Id::LIGHT_LUM:         return p1.light_lum.fmt(buffer);
            case Id::LIGHT_RGB:         return p1.light_rgb.fmt(buffer);
            case Id::TILT:              return p1.tilt.fmt(buffer);
            case Id::TEMP:              return p1.temp.fmt(buffer);
            case Id::POWER_STATE:
                return fmt_power_state(buffer, p1.power_state);
            case Id::HOTSPOT_STARTED:   return p1.hotspot_started.fmt(buffer);
            case Id::HOTSPOT_FAILED:    return p1.hotspot_failed.fmt(buffer);
            case Id::HOTSPOT_CLIENT_COUNT:
                return p1.hotspot_client_count.fmt(buffer);
        }
        lite::TextBuffer text(buffer);
        text.appendf("event %s", id.str());
        return buffer;
    }
};

static_assert(sizeof(Event) == 5, "unexpected size of event::Event");

//=============================================================================
#pragma pack(pop)

using Bus = lite::EventBus<Event>;
using Hook = lite::EventHook<Event>;
using Queue = lite::EventQueue<Event, EVENT_QUEUE_SIZE>;

} // namespace event
