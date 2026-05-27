//   arduino bootstrap code
//  
//   see LICENSE for terms

#if defined(LITEKIT_TESTS)
    #error use platformio test for LITEKIT_TESTS environments
#endif

#include "run_host.h"

//==============================================================================
//  public arduino sketch entry points
//------------------------------------------------------------------------------
void setup() {
    RunHost::instance().setup();
}

void loop() {
    RunHost::instance().loop();
}
