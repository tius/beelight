//  main application entry point
//
//  see LICENSE for terms

#include "settings.h"
#include "lite/serial_out.h"
#include "lite/log.h"
#include "lite/clock.h"
#include "lite/timer.h"

#define LOG_TAG         app
#define LOG_LEVEL       trace

using namespace std::chrono_literals;

using Timer        = lite::TimerT<1000>;
using TimerService = lite::TimerServiceT<1000>;

//==============================================================================
class App {
//------------------------------------------------------------------------------
public:
	static App& instance() noexcept {
		static App app;
		return app;
	}    

    void loop() {
        auto& ts = lite::SpinTimerService::instance();
        (void)ts.spin( lite::now_ms() );

        // auto& ts = TimerService::instance();
        // (void)ts.spin( lite::duration_ms{lite::now_ms()} );
    }

//------------------------------------------------------------------------------
private:
    using AppLogger = lite::CustomLogger<LOG_ANSI_COLOR, LOG_TIMESTAMP>;

    lite::SerialOut     serial_out_{MONITOR_SPEED};
    lite::StdOut        std_out_{serial_out_};
    AppLogger           logger_{serial_out_};

    Timer               timer_{MSG_BIND(this, on_timer)};

    App() {
        serial_out_.println("\n--- app started ---");
        LOG_INFO(APP_BANNER_TEXT);
        LOG_TRACE("SDK version: %s", ESP.getSdkVersion());

        auto ms = lite::now_ms();
        LOG_WARN("now_ms: %u ms", ms);

        timer_.start_periodic(1s);
    }

    // prevent copying
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void on_timer() {
        LOG_INFO("timer fired");
    }

};
