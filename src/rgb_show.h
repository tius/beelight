//  RgbShow — rgb led state machine
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"
#include "rgb_led.h"
#include "rgb_state.h"

#include "lite/io/log.h"
#include "lite/core/changed.h"
#include "lite/effects/morse.h"
#include "lite/effects/breath.h"

#define LOG_TAG		    rgb_ctrl
#define LOG_LEVEL	    trace

//=============================================================================
class RgbMorseBreath final : public lite::MorseCrtp<RgbMorseBreath> {
//-----------------------------------------------------------------------------
public:
    RgbMorseBreath(RgbLed& led) : led_(led) {}

    void out(bool on) {
        if (on)     led_.set(color_);
        else        led_.set(lite::k_rgb_black);
    }
    void on_start() {
        breath_.stop();
    }
    void on_unknown(char c) {
        switch (c) {
            //  set color
            case 'r':   color_ = lite::k_rgb_red; break;
            case 'g':   color_ = lite::k_rgb_green; break;
            case 'b':   color_ = lite::k_rgb_blue; break;

            //  breathing
            case '~':   breath_.start(24ms); break;
        }
    }
    
//-----------------------------------------------------------------------------
private:
    class Breath final : public lite::BreathCrtp<Breath> {
    public:
        Breath(RgbMorseBreath& rgb_morse) : rgb_morse_(rgb_morse) {}

        void out(u8 brightness) {
            rgb_morse_.breath_out(brightness);
        }
    private:
        RgbMorseBreath&   rgb_morse_;
    };

    RgbLed&     led_;    
    lite::Rgb8  color_;
    Breath      breath_{*this};

    void breath_out(u8 brightness) {
        led_.set(color_ * brightness);
    }
};


//=============================================================================
class RgbShow {
//-----------------------------------------------------------------------------
public:
    RgbShow(RgbLed& led) : led_(led) {}

    void set(u8 state) noexcept {
        if ( !lite::changed(state_, state) ) return;
        morse_.off();
        switch (state) {
            case RgbState::OFF:     morse_.play("gI"); break;
            case RgbState::CHARGE:  morse_.play("r~"); break;
            case RgbState::TEST:    morse_.play("b-"); break;
        }
	}

//-----------------------------------------------------------------------------
private:
    RgbLed&         led_;
    RgbMorseBreath  morse_{led_};
    u8              state_ = 0;
};

//=============================================================================
#undef LOG_TAG
#undef LOG_LEVEL

