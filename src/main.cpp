//   arduino bootstrap code
//  
//   see LICENSE for terms

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
