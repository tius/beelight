//   arduino bootstrap code
//  
//   see LICENSE for terms

#if defined(LITEKIT_TESTS)
    #error use platformio test for LITEKIT_TESTS environments
#endif

#include "app.h"

//==============================================================================
//  public arduino sketch entry points
//------------------------------------------------------------------------------
void setup() {
    App::instance();
}

void loop() {
    App::instance().loop();
}
