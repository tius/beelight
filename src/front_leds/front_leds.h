//	front led output
//
//	see LICENSE for terms

#pragma once

#include "lite/pixels/neo_pixel.h"
#include "lite/pixels/map.h"

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
};