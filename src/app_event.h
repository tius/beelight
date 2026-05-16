//  application event definitions
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/types.h"
#include "lite/core/fsm.h"

class AppEvent;

using lite::u8;
using lite::u16;
using lite::u32;

//=============================================================================
struct AppEventId {
    enum : u8 {
        NONE,
        ENTER 			= lite::fsm::event::ENTER,
        LEAVE 			= lite::fsm::event::LEAVE,
        TIMEOUT 		= lite::fsm::event::TIMEOUT,

        IR_RX           = lite::fsm::event::COUNT_,     
        LIGHT_STATE,
        LIGHT_DATA,                                          
    };

    u8 id = 0;
    u8 p1 = 0;

    constexpr AppEventId() = default;
    constexpr AppEventId(u8 v) : id(v) {}
    constexpr operator u8() const { return id; }

    const char* str() const {
        switch (id) {
            case NONE:                  return "NONE";
            case ENTER:                 return "ENTER";
            case LEAVE:                 return "LEAVE";
            case TIMEOUT:               return "TIMEOUT";
            case IR_RX:                 return "IR_RX";
            case LIGHT_STATE:           return "LIGHT_STATE";
            case LIGHT_DATA:            return "LIGHT_DATA";
            default:                    return "?";
        }
    }
};

//=============================================================================
struct AppEvent {
    using Id        = AppEventId;
    using Payload   = u32;

    Id      id = Id::NONE;
    Payload p1 = 0;    

	[[nodiscard]] constexpr explicit operator bool() const noexcept {
		return id != Id::NONE;
	}

    AppEvent() = default;

    //  parameter id passed as int for less casting 
    AppEvent(int id, Payload p1 = 0) : id(id), p1(p1) {}
};

//=============================================================================
