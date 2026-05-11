//  main application entry point
//
//  see LICENSE for terms

#include "settings.h"
#include "rgb_controller.h"

#include "lite/cmd.h"
#include "lite/console.h"
#include "lite/serial_out.h"
#include "lite/log.h"
#include "lite/clock.h"
#include "lite/timer.h"
#include "lite/sys/sys_cmd.h"

#define LOG_TAG         app
#define LOG_LEVEL       trace

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
		lite::Timer::spin( lite::now() );

		//  process serial input
		if (auto c = serial_.read(); c >= 0) {
			console_.process(char(c));
		}
	}

//------------------------------------------------------------------------------
private:
    using AppLogger = lite::CustomLogger<LOG_ANSI_COLOR, LOG_TIMESTAMP, LOG_LEVEL_PREFIX>;

    lite::Serial        serial_{MONITOR_SPEED};
    lite::SerialOut     serial_out_{serial_, "\n-----\n"};
    lite::StdOut        std_out_{serial_out_};
    AppLogger           logger_{serial_out_};

    lite::CmdShell      shell_{};
    lite::Console       console_{shell_, serial_out_};
    lite::sys::SysCmd   cmd_sys_{shell_};

    RgbLed              rgb_led_{};
    RgbController       rgb_controller_{rgb_led_};
    lite::Cmd           cmd_led_{shell_, "led", "set rgb status state", "<state>", METHOD_THIS(on_cmd_led_)
    };

    App() {
        lite::std_out->println(APP_BANNER_TEXT);
        console_.ready();
        rgb_controller_.set(RgbState::CHARGE);
    }

    // prevent copying
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void on_cmd_led_(lite::Out& out, lite::Args args) {
        (void)out;
        rgb_controller_.set( args.get_u16() );
    }
};
