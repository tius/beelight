//	rgb led control
//
//	header-only state driver for onboard rgb led
//
//	see LICENSE for terms

#pragma once

#include "settings.h"
#include "app_event.h"
#include "lite/sys/pwm.h"
#include "lite/color/gamma_lut.h"
#include "lite/color/rgb8.h"
#include "lite/core/event_bus.h"

using u8    = lite::u8;
using u16   = lite::u16;
using u32   = lite::u32;
using Rgb8  = lite::Rgb8;

//==============================================================================
class RgbLed final {
//-----------------------------------------------------------------------------
using EventBus = lite::EventBus<AppEvent>;
using EventHook = lite::EventHook<AppEvent>;
//------------------------------------------------------------------------------    
public:
	static constexpr u16 k_duty_max = 1023;

	RgbLed(EventBus& event_bus)
	: pwm_event_hook_(event_bus)
	{
		pwm_event_hook_.attach<&RgbLed::on_app_event_>(this);
		set(0, 0, 0);
	}

	void set(u8 red, u8 green, u8 blue) {
		set({ red, green, blue });
	}

	void set(const Rgb8& rgb) {
		current_rgb_ = rgb;
		if (!suspend_cnt_) {
			apply_rgb_(current_rgb_);
		}
	}

//------------------------------------------------------------------------------    
private:
	static constexpr u32 k_freq_hz_     = 1000;

	EventHook 	pwm_event_hook_;
	Rgb8 		current_rgb_ = lite::k_rgb_black;
	u8 			suspend_cnt_ = 0;

	void on_app_event_(const AppEvent& event) {
		switch (event.id) {
		case AppEvent::Id::PWM_SUSPEND:
			on_pwm_suspend_();
			break;

		case AppEvent::Id::PWM_RESUME:
			on_pwm_resume_();
			break;

		default:
			break;
		}
	}

	void on_pwm_suspend_() {
		if (suspend_cnt_ == 0) {
			apply_rgb_(lite::k_rgb_black);
		}

		if (suspend_cnt_ < 0xFFu) {
			++suspend_cnt_;
		}
	}

	void on_pwm_resume_() {
		if (suspend_cnt_ == 0) {
			return;
		}
		--suspend_cnt_;
		if (suspend_cnt_ == 0) {
			apply_rgb_(current_rgb_);
		}
	}

	void apply_rgb_(const Rgb8& rgb) {
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
				.freq_hz 		= k_freq_hz_,
				.active_lo 		= false,
			}
		> pwm_{};
	};

	LedPwm<GPIO_BUILTIN_LED_R> led_r_{};
	LedPwm<GPIO_BUILTIN_LED_G> led_g_{};
	LedPwm<GPIO_BUILTIN_LED_B> led_b_{};
};
