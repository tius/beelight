//  manage ir receiver
//
//  see LICENSE file for terms

#pragma once

#include "app_event.h"

#include "lite/io/log.h"
#include "lite/core/event_bus.h"

#define IR_RECEIVE_PIN              IR_RX_GPIO       
#define NO_LED_SEND_FEEDBACK_CODE
#include "TinyIRReceiver.hpp"

#define LOG_TAG         irrx
#define LOG_LEVEL       IR_RX_LOG

//==============================================================================
class IrRx {
//------------------------------------------------------------------------------    
public:
    explicit IrRx(lite::EventBus<AppEvent>& event_bus) noexcept
    : event_bus_(event_bus) {
        // Enables the interrupt generation on change of IR input signal
        initPCIInterruptForTinyIRReceiver(); 
    }

    void tick() {
        if (TinyIRReceiverDecode()) {
            // printTinyIRReceiverResultMinimal(&Serial);
            LOG_INFO( 
                "addr=0x%02X cmd=0x%02X flags=0x%02X", 
                TinyIRReceiverData.Address, 
                TinyIRReceiverData.Command, 
                TinyIRReceiverData.Flags 
            );   

            // p1 packs address and command: high16=addr low16=cmd
            event_bus_.publish ( {AppEventId::IR_RX, { .ir_rx = { 
                .addr   = TinyIRReceiverData.Address, 
                .cmd    = TinyIRReceiverData.Command, 
                .repeat = bool(TinyIRReceiverData.Flags & IRDATA_FLAGS_IS_REPEAT)
            }}} );
        }
    }

private:
    lite::EventBus<AppEvent>& event_bus_;
};

#undef LOG_TAG
#undef LOG_LEVEL