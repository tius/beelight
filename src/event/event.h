//  runtime event definitions
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"
#include "driver/ir_code.h"
#include "power/power_state.h"

#include "lite/core/types.h"
#include "lite/core/bits.h"
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
        BATTERY_INFO,
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
            case BATTERY_INFO:          return "BATTERY_INFO";
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

static_assert(sizeof(MorseCmd) <= 4, "unexpected size of event::MorseCmd");

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

static_assert(sizeof(IrRx) <= 4, "unexpected size of event::IrRx");

struct LightLum {
    u8      y;
    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("light_lum %u", y);
        return buffer;
    }
};

static_assert(sizeof(LightLum) <= 4, "unexpected size of event::LightLum");

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

static_assert(sizeof(LightRgb) <= 4, "unexpected size of event::LightRgb");

struct Temp {
    s16 celsius10;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("temp %d.%d°C", int(celsius10) / 10, int(celsius10) % 10);
        return buffer;
    }
};

static_assert(sizeof(Temp) <= 4, "unexpected size of event::Temp");

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

static_assert(sizeof(Tilt) <= 4, "unexpected size of event::Tilt");

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

static_assert(
    sizeof(HotspotStarted) <= 4,
    "unexpected size of event::HotspotStarted"
);

struct HotspotFailed {
    lite::esp8266::HotspotStatus status;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("hotspot_failed %s", status.str());
        return buffer;
    }
};

static_assert(
    sizeof(HotspotFailed) <= 4,
    "unexpected size of event::HotspotFailed"
);

struct HotspotClientCount {
    u8 count;

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("hotspot_client_count %u", count);
        return buffer;
    }
};

static_assert(
    sizeof(HotspotClientCount) <= 4,
    "unexpected size of event::HotspotClientCount"
);

struct BatteryInfo {
    s16 current_ma  = 0;
    u8 soc_percent  = 0;
    u8 flags        = 0;

    constexpr BatteryInfo() noexcept = default;

    constexpr BatteryInfo(
        u8 soc_percent,
        s16 current_ma,
        bool charging,
        bool discharging,
        bool low_soc,
        bool critical_soc
    ) noexcept
        : current_ma(current_ma)
        , soc_percent(soc_percent)
        , flags(VALID)
    {
        lite::set_bit(flags, CHARGING, charging);
        lite::set_bit(flags, DISCHARGING, discharging);
        lite::set_bit(flags, LOW_SOC, low_soc);
        lite::set_bit(flags, CRIT_SOC, critical_soc);
    }

    static constexpr BatteryInfo error() noexcept {
        BatteryInfo info = {};
        lite::set_bit(info.flags, GAUGE_ERROR);
        return info;
    }

    constexpr bool operator==(const BatteryInfo& other) const noexcept {
        return soc_percent == other.soc_percent
            && current_ma == other.current_ma
            && flags == other.flags;
    }

    template <size_t N>
    const char* fmt(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);

        if (lite::bit_is_hi(flags, GAUGE_ERROR)) {
            text.append("battery_info gauge_error");
            return buffer;
        }

        text.appendf(
            "battery_info soc=%u%% current=%+dmA",
            static_cast<unsigned>(soc_percent),
            static_cast<int>(current_ma)
        );
        if (lite::bit_is_hi(flags, CHARGING)) {
            text.append(" charging");
        }
        if (lite::bit_is_hi(flags, DISCHARGING)) {
            text.append(" discharging");
        }
        if (lite::bit_is_hi(flags, CRIT_SOC)) {
            text.append(" critical");
        }
        else if (lite::bit_is_hi(flags, LOW_SOC)) {
            text.append(" low");
        }

        return buffer;
    }

private:
    enum Flag : unsigned {
        VALID = 0,
        LOW_SOC,
        CRIT_SOC,
        CHARGING,
        DISCHARGING,
        GAUGE_ERROR = 7,
    };
};

static_assert(sizeof(BatteryInfo) == 4, "unexpected size of event::BatteryInfo");

static_assert(sizeof(PowerState) <= 4, "unexpected size of PowerState");

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
        BatteryInfo         battery_info;
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
            case Id::BATTERY_INFO:      return p1.battery_info.fmt(buffer);
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
