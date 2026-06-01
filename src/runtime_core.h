//  shared runtime services
//
//  see LICENSE for terms

#pragma once

#include "settings.h"
#include "back_led/back_show.h"
#include "boot/request.h"
#include "event/event.h"
#include "event/logger.h"
#include "front_leds/front_leds.h"
#include "power/battery.h"
#include "power/power.h"

#include "lite/cli/cmd.h"
#include "lite/cli/console.h"
#include "lite/io/serial_out.h"
#include "lite/io/log.h"
#include "lite/sys/clock.h"
#include "lite/core/compile_time.h"
#include "lite/core/timer.h"
#include "lite/cmd/sys_cmd.h"
#include "lite/cmd/twi_cmd.h"

#define LOG_TAG         runtime
#define LOG_LEVEL       trace

//==============================================================================
class RuntimeCore final {

using u32 = lite::u32;
using EventBus = event::Bus;
//------------------------------------------------------------------------------
public:
    explicit RuntimeCore(EventBus& event_bus)
        : event_bus_(event_bus)
    {
        if (!power_) {
            LOG_ERROR("power not available: %s", power_.status().str());
        }
        if (!battery_) {
            LOG_ERROR("battery not available: %s", battery_.status().str());
        }
    }

    RuntimeCore(const RuntimeCore&) = delete;
    RuntimeCore& operator=(const RuntimeCore&) = delete;

    void ready() {
        lite::std_out->println(APP_BANNER_TEXT);
        console_.ready();
    }

    void tick() {
        lite::Timer::spin(lite::now());

        (void)event_queue_.tick();

        if (auto c = uart_.rx(); c >= 0) {
            console_.process(char(c));
        }

        front_leds_.tick();
    }

    auto& event_bus() noexcept {
        return event_bus_;
    }

    auto& event_queue() noexcept {
        return event_queue_;
    }

    auto& shell() noexcept {
        return shell_;
    }

    auto& front_leds() noexcept {
        return front_leds_;
    }

    auto& twi() noexcept {
        return twi_;
    }

    lite::duration_ms next_timer_offset() noexcept {
        timer_offset_ += k_timer_offset_step;
        if (timer_offset_ >= k_timer_offset_period) {
            timer_offset_ -= k_timer_offset_period;
        }
        return lite::duration_ms{timer_offset_};
    }

//------------------------------------------------------------------------------
private:
    static constexpr u32 k_timer_offset_period  = 1000u;
    static constexpr u32 k_timer_offset_step    = 377u;

    static_assert(
        1 == lite::gcd_u32( k_timer_offset_period, k_timer_offset_step ),
        "timer offset step must be coprime with period"
    );

    using AppLogger = lite::CustomLogger<
        LOG_ANSI_COLOR,
        LOG_TIMESTAMP,
        LOG_LEVEL_PREFIX
    >;

    u32                 timer_offset_ = 0u;

    EventBus&           event_bus_;
    event::Queue        event_queue_ {event_bus_};

    lite::Uart          uart_       {MONITOR_SPEED};
    lite::SerialOut     serial_out_ {uart_, "\n-----\n"};
    lite::StdOut        std_out_    {serial_out_};
    AppLogger           logger_     {serial_out_};

    lite::CmdShell      shell_      {};
    lite::Console       console_    {shell_, serial_out_};

    event::Logger       event_logger_{event_bus_};

    lite::Twi           twi_        {I2C_SDA_GPIO, I2C_SCL_GPIO, I2C_CLOCK_HZ};
    lite::cmd::TwiCmd   twi_cmd_    {shell_, twi_};

    Battery             battery_    {twi_, event_bus_, next_timer_offset()};
    Power               power_      {twi_, event_bus_, next_timer_offset()};

    BackLed             back_led_   {};
    BackShow            back_show_  {back_led_, event_bus_};
    FrontLeds           front_leds_ {};

    //  shell commands
    lite::Cmd           cmd_hotspot_ {shell_, "hotspot", "run hotspot", "", METHOD_THIS(on_cmd_hotspot)};

    void on_cmd_hotspot(lite::Out& out, lite::Args args) {
        boot::reboot(boot::Mode::hotspot);
    }

    lite::Cmd           cmd_off_    {shell_, "off", "turn battery off", "", METHOD_THIS(on_cmd_off)};

    void on_cmd_off(lite::Out&, lite::Args) {
        front_leds_.power(false);
        power_.prepare_power_off();
        power_.enter_shipping_mode();
    }

    lite::cmd::SysCmd  cmd_sys_    {shell_};

};

#undef LOG_TAG
#undef LOG_LEVEL
