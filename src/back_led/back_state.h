//  back state definitions
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/types.h"

//=============================================================================
struct BackState {
	enum : lite::u8 {
		OFF,
		CHARGE,
		HOTSPOT,
		TEST,
		COUNT_
	};

	lite::u8 id = 0;

	constexpr BackState() = default;
	constexpr BackState(lite::u8 value) : id(value) {}
	constexpr operator lite::u8() const { return id; }

	[[nodiscard]] const char* str() const noexcept {
		switch (id) {
			case OFF:     return "OFF";
			case CHARGE:  return "CHARGE";
			case HOTSPOT: return "HOTSPOT";
			case TEST:    return "TEST";
			default:      return "?";
		}
	}
};