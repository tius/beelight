//	pixel stripe output
//
//	see LICENSE for terms

#pragma once

#include "settings.h"
#include "lite/stripe/neo.h"

//==============================================================================
class Stripe : public lite::stripe::Neo<NEO_GPIO, u8> {
//------------------------------------------------------------------------------    
public:
	Stripe() 
		: lite::stripe::Neo<NEO_GPIO, u8>(NEO_PIXEL_CNT * 3) {}
};