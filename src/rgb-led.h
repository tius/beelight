//	rgb led control
//
//	header-only state driver for onboard rgb led
//
//	see LICENSE for terms

#pragma once

#include "settings.h"
#include "lite/gamma_lut.h"
#include "lite/pwm.h"

using u8    = lite::u8;
using u16   = lite::u16;
using u32   = lite::u32;

//==============================================================================
class RgbLed final {
//------------------------------------------------------------------------------    
public:
	static constexpr u16 k_duty_max = 1023;

	RgbLed() {
		set(0, 0, 0);
	}

	void set(u16 red_duty, u16 green_duty, u16 blue_duty) {
		led_r_.set_duty(red_duty);
		led_g_.set_duty(green_duty);
		led_b_.set_duty(blue_duty);
	}

//------------------------------------------------------------------------------    
private:
	static constexpr u32 k_duty_levels_ = static_cast<u32>(k_duty_max) + 1u;
	static constexpr u32 k_freq_hz_     = 1000;

	template <u8 Pin>
	class LedPwm {
	public:
		void set_duty(u16 duty) {
			if (duty > k_duty_max) {
				duty = k_duty_max;
			}
			const auto gamma_duty = gamma_lut_t {}.lookup(
				static_cast<typename gamma_lut_t::input_type>(duty)
			);
			pwm_.set_duty(static_cast<u16>(gamma_duty));
		}

	private:
		using gamma_lut_t = lite::GammaLut<k_duty_levels_, k_duty_levels_>;

		lite::pwm::Pwm<
			Pin,
			lite::pwm::PwmConfig{
				.initial_duty = 0,
				.duty_max = k_duty_max,
				.freq_hz = k_freq_hz_,
				.active_lo = false,
			}
		> pwm_{};
	};

	LedPwm<GPIO_BUILTIN_LED_R> led_r_{};
	LedPwm<GPIO_BUILTIN_LED_G> led_g_{};
	LedPwm<GPIO_BUILTIN_LED_B> led_b_{};
};
