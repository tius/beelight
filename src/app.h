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

#define LOG_TAG         app
#define LOG_LEVEL       trace

using namespace std::chrono_literals;

using Timer        = lite::TimerT<1000>;

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
    using AppLogger = lite::CustomLogger<LOG_ANSI_COLOR, LOG_TIMESTAMP>;

    lite::Serial        serial_{MONITOR_SPEED};
    lite::SerialOut     serial_out_{serial_};
    lite::StdOut        std_out_{serial_out_};
    AppLogger           logger_{serial_out_};
    
    lite::CmdShell      shell_{};
    lite::Cmd           cmd_echo_{shell_, "echo", "echo remaining line", "[text]", METHOD_THIS(on_cmd_echo)};
    lite::Console       console_{shell_, serial_out_};

    // Timer               timer_{MSG_BIND(this, on_timer)};

    App() {
        serial_out_.println("\n--- app started ---");
        LOG_INFO(APP_BANNER_TEXT);
        LOG_TRACE("SDK version: %s", ESP.getSdkVersion());

        auto ms = lite::now_ms();
        LOG_WARN("now_ms: %u ms", ms);

        console_.ready();

        // timer_.start_periodic(1s);
    }

    // prevent copying
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void on_cmd_echo(lite::Out& out, lite::Args args) {
        if (args.is_empty()) {
            out.crlf();
            return;
        }

        out.println(args.as_str());
    }

    // void on_timer() {
    //     LOG_INFO("timer fired");
    // }

};
