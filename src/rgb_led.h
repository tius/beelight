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

//==============================================================================
class RgbLed final {
//------------------------------------------------------------------------------    
public:
	static constexpr u16 k_duty_max = 1023;

	RgbLed() {
		set(0, 0, 0);
	}

	void set(u8 red, u8 green, u8 blue) {
		led_r_.set(red);
		led_g_.set(green);
		led_b_.set(blue);
	}

	void set(const lite::Rgb8& rgb) {
		set(rgb.r, rgb.g, rgb.b);
	}

//------------------------------------------------------------------------------    
private:
	static constexpr u32 k_freq_hz_     = 1000;

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
				.freq_hz 		= k_freq_hz_,
				.active_lo 		= false,
			}
		> pwm_{};
	};

	LedPwm<GPIO_BUILTIN_LED_R> led_r_{};
	LedPwm<GPIO_BUILTIN_LED_G> led_g_{};
	LedPwm<GPIO_BUILTIN_LED_B> led_b_{};
};
