//	rgb led control
//
//	header-only state driver for onboard rgb led
//
//	see LICENSE for terms

#pragma once

#include "settings.h"
#include "lite/sys/pwm.h"
#include "lite/color/gamma_lut.h"
#include "lite/color/rgb8.h"

using u8    = lite::u8;
using u16   = lite::u16;
using u32   = lite::u32;
using Rgb8  = lite::Rgb8;

//==============================================================================
class RgbLed final {
public:
	static constexpr u16 k_duty_max = 1023;

	RgbLed() {
		set(0, 0, 0);
	}

	void set(u8 red, u8 green, u8 blue) {
		set({ red, green, blue });
	}

	void set(const Rgb8& rgb) {
		current_rgb_ = rgb;
		apply_rgb(current_rgb_);
	}

//------------------------------------------------------------------------------    
private:
	static constexpr u32 k_freq_hz      = 1000;

	Rgb8 		current_rgb_ = lite::k_rgb_black;

	void apply_rgb(const Rgb8& rgb) {
		led_r_.set(rgb.r);
		led_g_.set(rgb.g);
		led_b_.set(rgb.b);
	}

	template <u8 Pin>
	class LedPwm {
	public:
		void set(u8 duty) {
			const auto gamma_duty = lite::gamma_u8_to_u10(duty);
			pwm_.set_duty(static_cast<u16>(gamma_duty));
		}

		lite::pwm::Pwm<
			Pin,
			lite::pwm::PwmCfg{
				.initial_duty 	= 0,
				.duty_max 		= k_duty_max,
				.freq_hz 		= k_freq_hz,
				.active_lo 		= false,
			}
		> pwm_{};
	};

	LedPwm<GPIO_BUILTIN_LED_R> led_r_{};
	LedPwm<GPIO_BUILTIN_LED_G> led_g_{};
	LedPwm<GPIO_BUILTIN_LED_B> led_b_{};
};
