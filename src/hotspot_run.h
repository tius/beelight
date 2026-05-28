//  hotspot runtime
//
//  see LICENSE for terms

#pragma once

#include <cstdio>

#include <ESP8266WiFi.h>

#include "back_led/back_state.h"
#include "runtime_core.h"

#include "lite/io/log.h"
#include "lite/sys/device_info.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#ifdef LOG_LEVEL
#undef LOG_LEVEL
#endif

#define LOG_TAG         hotspot
#define LOG_LEVEL       trace

//=============================================================================
class HotspotRun final {
//-----------------------------------------------------------------------------
public:
    HotspotRun() {
        core_.ready();
        core_.back_show().set(BackState::HOTSPOT);
        start_hotspot();
    }

    void loop() {
        core_.loop();
    }

//-----------------------------------------------------------------------------
private:
    static constexpr auto DEVICE_ID_HEX_LEN = 6;
    static constexpr auto SSID_SIZE =
        sizeof(HOTSPOT_SSID_PREFIX) + DEVICE_ID_HEX_LEN;

    static_assert(SSID_SIZE <= 33);
    static_assert(sizeof(HOTSPOT_PSK) >= 9);
    static_assert(sizeof(HOTSPOT_PSK) <= 64);

    RuntimeCore core_ {};
    char ssid_[SSID_SIZE] {};

    void start_hotspot() {
        format_ssid();

        WiFi.persistent(false);

        if (!WiFi.mode(WIFI_AP)) {
            LOG_ERROR("wifi: ap mode failed");
            return;
        }

        if (!WiFi.softAP(ssid_, HOTSPOT_PSK)) {
            LOG_ERROR("access point: start failed");
            return;
        }

        const auto ip = WiFi.softAPIP();
        LOG_INFO(
            "access point: started ssid=%s ip=%u.%u.%u.%u",
            ssid_,
            static_cast<unsigned>(ip[0]),
            static_cast<unsigned>(ip[1]),
            static_cast<unsigned>(ip[2]),
            static_cast<unsigned>(ip[3])
        );
    }

    void format_ssid() {
        std::snprintf(
            ssid_,
            sizeof(ssid_),
            "%s%06lx",
            HOTSPOT_SSID_PREFIX,
            static_cast<unsigned long>(lite::sys::device_id())
        );
    }
};

#undef LOG_LEVEL
#undef LOG_TAG
