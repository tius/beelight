//  manage ir receiver
//
//  see LICENSE file for terms

#pragma once
#include "lite/io/log.h"

#define IR_RECEIVE_PIN              IR_RX_GPIO       
#define NO_LED_SEND_FEEDBACK_CODE
#include "TinyIRReceiver.hpp"

#define LOG_TAG         irrx
#define LOG_LEVEL       info

//==============================================================================
class IrRx {
//------------------------------------------------------------------------------    
public:
    IrRx() {
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
        }
    }
};

#undef LOG_TAG
#undef LOG_LEVEL