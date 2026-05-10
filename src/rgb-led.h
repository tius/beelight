//	rgb led control
//
//	header-only state driver for onboard rgb led
//
//	see LICENSE for terms

#pragma once

#include "settings.h"
#include "lite/gamma_lut.h"
#include "lite/pwm.h"
#include "lite/timer.h"

#include <chrono>

using namespace std::chrono_literals;

using u8    = lite::u8;
using u16   = lite::u16;
using u32   = lite::u32;

//==============================================================================
class RgbLed final {
//------------------------------------------------------------------------------    
public:
	enum State : u8 {
		OFF,
		CHARGE,
	};

	RgbLed() {
		set_all_off_();
		timer_.start_periodic(k_tick_);
	}

	void set(u8 state) {
		state_ = normalize_state_(state);

		switch (state_) {
			case OFF:
				set_all_off_();
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
	using Timer = lite::TimerT<1000>;

	static constexpr auto k_tick_       = 15ms;
	static constexpr u16 k_duty_max_    = 1023;
	static constexpr u32 k_duty_levels_ = static_cast<u32>(k_duty_max_) + 1u;
	static constexpr u32 k_freq_hz_     = 1000;
	static constexpr u16 k_fade_step_   = 8;

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

	template <u8 Pin>
	class LedPwm {
	public:
		void off() {
			pwm_.off();
		}

		void set_duty(u16 duty) {
			if (duty > k_duty_max_) {
				duty = k_duty_max_;
			}

			const auto gamma_duty = gamma_lut_t {}.lookup(
				static_cast<typename gamma_lut_t::input_type>(duty)
			);
			pwm_.set_duty(static_cast<u16>(gamma_duty));
		}

	private:
		using gamma_lut_t = lite::Lut<k_duty_levels_, k_duty_levels_>;

		lite::pwm::Pwm<
			Pin,
			lite::pwm::PwmConfig{
				.initial_duty = 0,
				.duty_max = k_duty_max_,
				.freq_hz = k_freq_hz_,
				.active_lo = false,
			}
		> pwm_{};
	};

	void set_all_off_() {
		led_r_.off();
		led_g_.off();
		led_b_.off();
	}

	void reset_fade_() {
		led_r_duty_ = 0;
		led_r_fade_up_ = true;
	}

	void apply_charge_() {
		if (led_r_fade_up_) {
			if (led_r_duty_ >= k_duty_max_ - k_fade_step_) {
				led_r_duty_ = k_duty_max_;
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

		led_r_.set_duty(led_r_duty_);
		led_g_.off();
		led_b_.off();
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

	Timer timer_{MSG_BIND(this, on_timer_)};
	State state_ = OFF;

	LedPwm<GPIO_BUILTIN_LED_R> led_r_{};
	LedPwm<GPIO_BUILTIN_LED_G> led_g_{};
	LedPwm<GPIO_BUILTIN_LED_B> led_b_{};

	lite::u16 led_r_duty_ = 0;
	bool led_r_fade_up_ = true;
};
