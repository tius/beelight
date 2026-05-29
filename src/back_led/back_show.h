//  back show state machine
//
//  see LICENSE file for terms

#pragma once

#include "back_led.h"
#include "event/event.h"
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
		led_.set(on ? color_ : lite::k_rgb_black);
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
	lite::Rgb8 color_ = lite::k_rgb_black;
	Breath     breath_{*this};

	void breath_out(lite::u8 brightness) {
		led_.set(color_ * brightness);
	}
};

//=============================================================================
class BackShow final {
//-----------------------------------------------------------------------------
public:
	BackShow(BackLed& led, event::Bus& event_bus)
		: event_hook_(event_bus), morse_(led) {
		event_hook_.attach<&BackShow::on_event>(this);
		apply_state(state_);
	}

//-----------------------------------------------------------------------------
private:
	struct State {
		enum : lite::u8 {
			idle,
			hotspot_clients_0,
			hotspot_clients_1,
			hotspot_clients_2,
			hotspot_clients_3,
			hotspot_clients_4,
			hotspot_failed,
		};

		lite::u8 id = idle;

		constexpr State() = default;
		constexpr State(lite::u8 value) : id(value) {}
		constexpr operator lite::u8() const { return id; }

		const char* code() const noexcept {
			switch (id) {
				case idle:              return "r~";
				case hotspot_clients_0: return "b~";
				case hotspot_clients_1: return "bE    \r";
				case hotspot_clients_2: return "bI    \r";
				case hotspot_clients_3: return "bS    \r";
				case hotspot_clients_4: return "bH    \r";
				case hotspot_failed:    return "bT";
				default:                return "r~";
			}
		}
	};

	void on_event(const event::Event& event) {
		switch (event.id) {
			case event::Id::HOTSPOT_STARTED:
				set_state(State::hotspot_clients_0);
				break;

			case event::Id::HOTSPOT_FAILED:
				set_state(State::hotspot_failed);
				break;

			case event::Id::HOTSPOT_CLIENT_COUNT:
				set_state( hotspot_clients_state(
					event.p1.hotspot_client_count.count
				) );
				break;
		}
	}

	void set_state(State state) {
		if ( lite::changed(state_, state) ) {
			apply_state(state_);
		}
	}

	void apply_state(State state) {
		morse_.off();
		morse_.play(state.code());
	}

	static constexpr State hotspot_clients_state(lite::u8 count) {
		switch (count) {
			case 0u:  return State::hotspot_clients_0;
			case 1u:  return State::hotspot_clients_1;
			case 2u:  return State::hotspot_clients_2;
			case 3u:  return State::hotspot_clients_3;
			default:  return State::hotspot_clients_4;
		}
	}

	event::Hook      event_hook_;
	BackMorseBreath  morse_;
	State            state_ = State::idle;
};

//=============================================================================
#undef LOG_TAG
#undef LOG_LEVEL