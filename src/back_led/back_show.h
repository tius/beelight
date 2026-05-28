//  back show state machine
//
//  see LICENSE file for terms

#pragma once

#include "back_led.h"
#include "back_state.h"
#include "settings.h"

#include "lite/core/changed.h"
#include "lite/effects/morse.h"
#include "lite/effects/breath.h"
#include "lite/io/log.h"

#define LOG_TAG     back_show
#define LOG_LEVEL   trace

//=============================================================================
class BackMorseBreath final : public lite::MorseCrtp<BackMorseBreath> {
//-----------------------------------------------------------------------------
public:
	explicit BackMorseBreath(BackLed& led)
		: led_(led) {}

	void out(bool on) {
		if (on) {
			led_.set(color_);
			return;
		}

		led_.set(lite::k_rgb_black);
	}

	void on_start() {
		breath_.stop();
	}

	void on_unknown(char value) {
		switch (value) {
			case 'r': color_ = lite::k_rgb_red; break;
			case 'g': color_ = lite::k_rgb_green; break;
			case 'b': color_ = lite::k_rgb_blue; break;

			case '~': breath_.start(24ms); break;
		}
	}

//-----------------------------------------------------------------------------
private:
	class Breath final : public lite::BreathCrtp<Breath> {
	public:
		explicit Breath(BackMorseBreath& morse)
			: morse_(morse) {}

		void out(lite::u8 brightness) {
			morse_.breath_out(brightness);
		}

	private:
		BackMorseBreath& morse_;
	};

	BackLed&   led_;
	lite::Rgb8 color_;
	Breath     breath_{*this};

	void breath_out(lite::u8 brightness) {
		led_.set(color_ * brightness);
	}
};

//=============================================================================
class BackShow final {
//-----------------------------------------------------------------------------
public:
	explicit BackShow(BackLed& led)
		: led_(led) {}

	void set(lite::u8 state) noexcept {
		if (!lite::changed(state_, state)) {
			return;
		}

		apply_state(state_);
	}

//-----------------------------------------------------------------------------
private:
	void apply_state(lite::u8 state) noexcept {
		morse_.off();

		switch (state) {
			case BackState::OFF:     morse_.play("gI"); break;
			case BackState::CHARGE:  morse_.play("r~"); break;
			case BackState::HOTSPOT: morse_.play("bHS\r"); break;
			case BackState::TEST:    morse_.play("b-"); break;
		}
	}

	BackLed&         led_;
	BackMorseBreath  morse_{led_};
	lite::u8         state_ = 0;
};

//=============================================================================
#undef LOG_TAG
#undef LOG_LEVEL