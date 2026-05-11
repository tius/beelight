//  RgbController — rgb led state machine
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"
#include "rgb_led.h"
#include "rgb_state.h"
#include "app_event.h"

#include "lite/log.h"
#include "lite/fsm.h"
#include "lite/fsm_data.h"

#define LOG_TAG		rgb_ctrl
#define LOG_LEVEL	trace

//=============================================================================
//  main application controller FSM
//-----------------------------------------------------------------------------
class RgbController final 
    : public lite::fsm::TimerFsm<RgbController, RgbState, AppEvent> {
//-----------------------------------------------------------------------------
public:
    // construct and start in SPLASH state
    RgbController(RgbLed& led) noexcept
        : led_(led)
    {
        transition_to(RgbState::OFF);
    }

    void set(u8 state) noexcept {
        state = std::min(state, static_cast<u8>(RgbState::_COUNT - 1));

		if (state != fsm_state()) {
			transition_to(state);
		}
	}
    
    // dispatch incoming event to the active state handler
    void on_event(AppEvent event) noexcept {
        auto state = fsm_state();
        LOG_INFO("%s: %s p1=%d", state.str(), AppEventId(event.id).str(), event.p1);

        switch (state) {
            case RgbState::OFF:         return handle_off(event);
            case RgbState::CHARGE:      return handle_charge(event);
        }
    }

//-----------------------------------------------------------------------------
private:
    using Id = AppEventId;

    RgbLed led_;

    void handle_off(AppEvent event) noexcept {
        switch (event.id) {
            case Id::ENTER:
                led_.set(0, 0, 0);
                break;
        }           
    }

    void handle_charge(AppEvent event) noexcept {
        switch (event.id) {
            case Id::ENTER:
                LOG_INFO("entering CHARGE state");
                led_.set(127, 0, 0);
                break;
        }           
    }

};

//=============================================================================
#undef LOG_TAG
#undef LOG_LEVEL

