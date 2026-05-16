//  manage ir transmitter
//
//  see LICENSE file for terms

#pragma once
#include "lite/io/log.h"
#include "lite/core/timer.h"

#define IR_SEND_PIN                 IR_TX_GPIO
#define NO_LED_SEND_FEEDBACK_CODE
#include "TinyIRSender.hpp"

#define LOG_TAG         irtx
#define LOG_LEVEL       IR_TX_LOG

//==============================================================================
class IrTx {
//------------------------------------------------------------------------------    
public:
    IrTx() {
        timer_.start_periodic(1s);
    }

//------------------------------------------------------------------------------    
private:
    lite::Timer timer_      { MSG_THIS(on_timer_) };
    u8          cnt_        = 0;;

    void on_timer_() {
        if (++cnt_ == 20) {
            timer_.stop();
        }

        sendNEC(
            IR_SEND_PIN, 
            0x0000,                     // address
            cnt_ % 2 ? 0x0D : 0x1F,     // command
            0                           // number of repeats
        ); 
    }    
};

#undef LOG_TAG
#undef LOG_LEVEL