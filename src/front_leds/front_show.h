//  front show
//
//  see LICENSE for terms

#pragma once

#include "front_leds/front_leds.h"

#include "lite/core/timer.h"

//==============================================================================
class FrontShow final {
//------------------------------------------------------------------------------
public:
    explicit FrontShow(FrontLeds& front_leds)
        : front_leds_(front_leds), timer_(MSG_THIS(on_timer)) {}

    void start() {
        anim_step_idx_ = 0u;
        anim_pixel_idx_ = 0u;
        timer_.start_periodic(k_anim_step);
        draw_step();
    }

//------------------------------------------------------------------------------
private:
    static constexpr lite::duration_ms k_anim_step = lite::duration_ms{100};
    static constexpr lite::u16 k_anim_steps = 50u;

    FrontLeds& front_leds_;
    lite::Timer timer_;
    lite::u16 anim_step_idx_ = 0u;
    lite::u16 anim_pixel_idx_ = 0u;

    void on_timer() {
        if (anim_step_idx_ >= k_anim_steps) {
            timer_.stop();
            front_leds_.clr();
            front_leds_.update();
            return;
        }

        draw_step();
    }

    void draw_step() {
        front_leds_.clr();
        front_leds_.set(anim_pixel_idx_, lite::k_rgb_red);
        front_leds_.update();

        ++anim_step_idx_;
        ++anim_pixel_idx_;
        if (anim_pixel_idx_ >= NEO_PIXEL_CNT) {
            anim_pixel_idx_ = 0u;
        }
    }
};