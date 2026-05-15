//  application event definitions
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/types.h"
#include "lite/core/fsm.h"

class AppEvent;

using lite::u8;
using lite::u16;

//=============================================================================
struct AppEventId {
    enum : u8 {
        NONE,
        ENTER 			    = lite::fsm::event::ENTER,
        LEAVE 			    = lite::fsm::event::LEAVE,
        TIMEOUT 		    = lite::fsm::event::TIMEOUT,
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
            default:                    return "?";
        }
    }
};

//=============================================================================
struct AppEvent {
    using Id        = AppEventId;
    using Payload   = u8;

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
