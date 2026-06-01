//	front led output
//
//	remarks:
//		- a high-side driver is used to control power of the front leds
//		- to switch off the front leds, the data line must be held high
//
//	see LICENSE for terms

#pragma once

#include <Arduino.h>

#include "lite/pixels/neo_pixel.h"
#include "lite/pixels/map.h"
#include "lite/core/changed.h"

using FrontLedsBase = lite::pixels::map::NEO_COLOR_ORDER<
	lite::pixels::NeoPixel<NEO_GPIO>
>;

//==============================================================================
class FrontLeds final : public FrontLedsBase {
//------------------------------------------------------------------------------    
public:
	FrontLeds()
		: FrontLedsBase(NEO_PIXEL_CNT) {
		clr();
		update();
	}

	void power(bool on) {
		if ( !lite::changed(power_on_, on) ) {
			return;
		}
		if (on) {
			::pinMode(NEO_GPIO, SPECIAL);
			resume();
		}
		else {
			suspend();
			::pinMode(NEO_GPIO, INPUT);
		}
	}

	bool tick() {
		if (power_on_) {
			return FrontLedsBase::tick();
		}
		else {
			return false;
		}
	}

//------------------------------------------------------------------------------
private:
	bool power_on_ 	= true;
};