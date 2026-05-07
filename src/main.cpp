#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  delay(1000);

  // put your setup code here, to run once:
  Serial.begin(74880);
  Serial.println("\nSDK version: " + String(ESP.getSdkVersion()));
}

void loop() {
  // put your main code here, to run repeatedly:
}
