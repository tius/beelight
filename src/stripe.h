//	pixel stripe output
//
//	see LICENSE for terms

#pragma once

#include "lite/io/log.h"
#include "lite/core/timer.h"
#include "lite/stripe/neo_pixel.h"
#include "lite/stripe/map.h"

#define LOG_TAG         stripe
#define LOG_LEVEL       info

using namespace lite::stripe;

using StripeBase = map::NEO_COLOR_ORDER<NeoPixel<NEO_GPIO>>;

//==============================================================================
class Stripe : public StripeBase {
//------------------------------------------------------------------------------    
public:
	Stripe()
		: StripeBase(NEO_PIXEL_CNT)
		, timer_(MSG_THIS(on_timer)) {
		clr(lite::k_rgb_black);
		update();
		timer_.start_periodic(k_anim_step);
	}

//------------------------------------------------------------------------------
private:
	static constexpr lite::duration_ms k_anim_step = lite::duration_ms{100};
	static constexpr lite::u16 k_anim_steps = 50u;

	lite::Timer timer_;
	lite::u16 anim_step_idx_ = 0u;
	lite::u16 anim_pixel_idx_ = 0u;

	void on_timer() {
		if (anim_step_idx_ >= k_anim_steps) {
			timer_.stop();
			clr(lite::k_rgb_black);
			update();
			return;
		}

		clr(lite::k_rgb_black);
		set(anim_pixel_idx_, lite::k_rgb_red);
		update();

		++anim_step_idx_;
		++anim_pixel_idx_;
		if (anim_pixel_idx_ >= NEO_PIXEL_CNT) {
			anim_pixel_idx_ = 0u;
		}
	}
};

#undef LOG_TAG
#undef LOG_LEVEL