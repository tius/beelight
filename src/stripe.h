//	pixel stripe output
//
//	see LICENSE for terms

#pragma once

#include "settings.h"
#include "lite/stripe/neo.h"
#include "lite/stripe/mapping.h"

using namespace lite::stripe;

using StripeBase = Mapping::NEO_COLOR_ORDER<Neo<NEO_GPIO>>;

//==============================================================================
class Stripe : public StripeBase {
//------------------------------------------------------------------------------    
public:
	Stripe() 
		: StripeBase(NEO_PIXEL_CNT) {}
};