//  main application entry point
//
//  see LICENSE for terms

#include "settings.h"
#include "lite/serial_out.h"
#include "lite/log.h"
#include "lite/clock.h"

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
        // put your main code here, to run repeatedly
    }

//------------------------------------------------------------------------------
private:
    using AppLogger = lite::CustomLogger<LOG_ANSI_COLOR, LOG_TIMESTAMP>;

    lite::SerialOut     serial_out_{MONITOR_SPEED};
    lite::StdOut        std_out_{serial_out_};
    AppLogger           logger_{serial_out_};

    App() {
        serial_out_.println("\n--- app started ---");
        LOG_INFO(APP_BANNER_TEXT);
        LOG_TRACE("SDK version: %s", ESP.getSdkVersion());

        auto ms = lite::now_ms();
        LOG_WARN("now_ms: %u ms", ms);
    }

    // prevent copying
    App(const App&) = delete;
    App& operator=(const App&) = delete;

};
