//	rgb status animations
//
//	header-only animation driver for onboard rgb led states
//
//	see LICENSE for terms

#pragma once

#include "rgb-led.h"
#include "lite/timer.h"

#include <chrono>

using namespace std::chrono_literals;

//==============================================================================
class RgbStatus final {
//------------------------------------------------------------------------------
public:
	enum State : u8 {
		OFF,
		CHARGE,
	};

	explicit RgbStatus(RgbLed& led)
		: led_(led) {
		set_led_off_();
		timer_.start_periodic(k_tick_);
	}

	void set(u8 state) {
		state_ = normalize_state_(state);

		switch (state_) {
			case OFF:
				set_led_off_();
				reset_fade_();
				return;

			case CHARGE:
				on_timer_();
				return;

			default:
				return;
		}
	}

//------------------------------------------------------------------------------
private:
	static constexpr auto k_tick_     = 15ms;
	static constexpr u16 k_fade_step_ = 8;

	static constexpr State normalize_state_(u8 state) {
		switch (static_cast<State>(state)) {
			case OFF:
				return OFF;

			case CHARGE:
				return CHARGE;

			default:
				return OFF;
		}
	}

	void set_led_off_() {
		led_.set(0, 0, 0);
	}

	void reset_fade_() {
		led_r_duty_ = 0;
		led_r_fade_up_ = true;
	}

	void apply_charge_() {
		if (led_r_fade_up_) {
			if (led_r_duty_ >= RgbLed::k_duty_max - k_fade_step_) {
				led_r_duty_ = RgbLed::k_duty_max;
				led_r_fade_up_ = false;
			} else {
				led_r_duty_ =
					static_cast<u16>(led_r_duty_ + k_fade_step_);
			}
		} else {
			if (led_r_duty_ <= k_fade_step_) {
				led_r_duty_ = 0;
				led_r_fade_up_ = true;
			} else {
				led_r_duty_ =
					static_cast<u16>(led_r_duty_ - k_fade_step_);
			}
		}

		led_.set(led_r_duty_, 0, 0);
	}

	void on_timer_() {
		switch (state_) {
			case OFF:
				return;

			case CHARGE:
				apply_charge_();
				return;

			default:
				return;
		}
	}

	RgbLed& led_;
	lite::Timer timer_{MSG_BIND(this, on_timer_)};
	State state_ = OFF;

	u16 led_r_duty_ = 0;
	bool led_r_fade_up_ = true;
};
