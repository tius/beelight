//  hotspot runtime
//
//  see LICENSE for terms

#pragma once

#include "event/event.h"
#include "runtime_core.h"

#include "lite/core/changed.h"
#include "lite/core/text_buffer.h"
#include "lite/esp8266/hotspot.h"
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

        setup_ssid();
        start_hotspot();
    }

    void loop() {
        core_.tick();
        tick_hotspot();
    }

//-----------------------------------------------------------------------------
private:
    static constexpr auto DEVICE_ID_HEX_LEN = 6;
    static constexpr auto SSID_SIZE =
        sizeof(HOTSPOT_SSID_PREFIX) + DEVICE_ID_HEX_LEN;

    static_assert(SSID_SIZE <= 33);
    static_assert(sizeof(HOTSPOT_PSK) >= 9);
    static_assert(sizeof(HOTSPOT_PSK) <= 64);

    static constexpr lite::esp8266::HotspotDhcpConfig DHCP_CFG {
        .ip             = HOTSPOT_IP,
        .gateway        = HOTSPOT_GATEWAY,
        .netmask        = HOTSPOT_NETMASK,
        .lease_start    = HOTSPOT_LEASE_START,
        .lease_end      = HOTSPOT_LEASE_END,
    };

    RuntimeCore core_ {};
    lite::esp8266::Hotspot hotspot_ {};
    char ssid_[SSID_SIZE] {};
    lite::u8 client_count_ = 0u;
    bool started_ = false;

    void start_hotspot() {
        const auto status = hotspot_.start(ssid_, HOTSPOT_PSK, DHCP_CFG);
        if (status.is_error()) {
            LOG_ERROR("start failed: %s", status.str());
            post_failed(status);
            return;
        }

        started_ = true;
        client_count_ = hotspot_.client_count();

        const auto ip = hotspot_.ip();
        char ip_buf[16];
        LOG_INFO(
            "started ssid=%s ip=%s",
            ssid_,
            ip.fmt(ip_buf)
        );

        post_started(ip);
        post_client_count(client_count_);
    }

    bool tick_hotspot() {
        if ( started_ && lite::changed(client_count_, hotspot_.client_count()) ) {
            post_client_count(client_count_);
            return true;
        }
        return false;
    }

    void setup_ssid() {
        lite::TextBuffer text(ssid_);
        text.appendf(
            "%s%06lx",
            HOTSPOT_SSID_PREFIX,
            static_cast<unsigned long>(lite::sys::device_id())
        );
    }

    void post_started(lite::Ipv4 ip) {
        post({ event::Id::HOTSPOT_STARTED, { .hotspot_started = { ip } } });
    }

    void post_failed(lite::esp8266::HotspotStatus status) {
        post({ event::Id::HOTSPOT_FAILED, { .hotspot_failed = { status } } });
    }

    void post_client_count(lite::u8 count) {
        post({ event::Id::HOTSPOT_CLIENT_COUNT, { .hotspot_client_count = {
            count
        } } });
    }

    void post(const event::Event& event) {
        if (core_.event_queue().post(event)) {
            return;
        }

        char buffer[96];
        LOG_ERROR("event queue: dropped %s", event.fmt(buffer));
    }

};

#undef LOG_LEVEL
#undef LOG_TAG
