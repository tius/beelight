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
                "addr=0x%04X cmd=0x%04X flags=0x%02X", 
                TinyIRReceiverData.Address, 
                TinyIRReceiverData.Command, 
                TinyIRReceiverData.Flags 
            );   

            // p1 packs address and command: high16=addr low16=cmd
            const u32 payload =
                (static_cast<u32>(TinyIRReceiverData.Address) << 16u)
                | static_cast<u32>(TinyIRReceiverData.Command);
            event_bus_.publish ( {AppEventId::IR_RX, payload} );
        }
    }

private:
    lite::EventBus<AppEvent>& event_bus_;
};

#undef LOG_TAG
#undef LOG_LEVEL