//  main application entry point
//
//  see LICENSE for terms

#include "settings.h"
#include "lite/cmd.h"
#include "lite/console.h"
#include "lite/serial_out.h"
#include "lite/log.h"
#include "lite/clock.h"
#include "lite/timer.h"
#include "lite/sys/sys_cmd.h"
#include "lite/gpio.h"
#include "lite/pwm.h"

#define LOG_TAG         app
#define LOG_LEVEL       trace

using namespace std::chrono_literals;

using Timer        = lite::TimerT<1000>;
using LedRPwm      = lite::pwm::Pwm<
    GPIO_BUILTIN_LED_R,
    lite::pwm::PwmConfig{
        .initial_duty = 0,
        .duty_max     = 1023,
        .freq_hz      = 1000,
        .active_lo    = false,
    }
>;

//==============================================================================
class App {
//------------------------------------------------------------------------------
public:
	static App& instance() noexcept {
		static App app;
		return app;
	}    

    void loop() {
        //  execute due timers 
        auto& ts = lite::SpinTimerService::instance();
        (void)ts.spin( lite::now_ms() );
        
        //  process serial input
        if ( auto c = serial_.read(); c >= 0 ) {
            console_.process( char(c) ) ;
        }
    }

//------------------------------------------------------------------------------
private:
    using AppLogger = lite::CustomLogger<LOG_ANSI_COLOR, LOG_TIMESTAMP, LOG_LEVEL_PREFIX>;

    static constexpr auto     led_r_fade_tick_ = 15ms;
    static constexpr lite::u16 led_r_fade_step_ = 8;

    lite::Serial        serial_{MONITOR_SPEED};
    lite::SerialOut     serial_out_{serial_, "\n-----\n"};
    lite::StdOut        std_out_{serial_out_};
    AppLogger           logger_{serial_out_};

    lite::CmdShell      shell_{};
    lite::Console       console_{shell_, serial_out_};
    lite::sys::SysCmd   cmd_sys_{shell_};

    Timer               timer_{MSG_BIND(this, on_timer)};

    LedRPwm                              pwm_led_r_;
    lite::gpio::Output<GPIO_BUILTIN_LED_G> gpio_led_g_;
    lite::gpio::Output<GPIO_BUILTIN_LED_B> gpio_led_b_;

    lite::u16                           led_r_duty_ = 0;
    bool                                led_r_fade_up_ = true;


    App() {
        lite::std_out->println(APP_BANNER_TEXT);
        console_.ready();

        timer_.start_periodic(led_r_fade_tick_);
    }

    // prevent copying
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void on_timer() {
        if (led_r_fade_up_) {
            if (led_r_duty_ >= LedRPwm::duty_max() - led_r_fade_step_) {
                led_r_duty_ = LedRPwm::duty_max();
                led_r_fade_up_ = false;
            } else {
                led_r_duty_ = static_cast<lite::u16>(led_r_duty_ + led_r_fade_step_);
            }
        } else {
            if (led_r_duty_ <= led_r_fade_step_) {
                led_r_duty_ = 0;
                led_r_fade_up_ = true;
            } else {
                led_r_duty_ = static_cast<lite::u16>(led_r_duty_ - led_r_fade_step_);
            }
        }

        pwm_led_r_.set_duty(led_r_duty_);
    }

};
