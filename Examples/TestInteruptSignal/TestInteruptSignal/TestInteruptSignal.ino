
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#define INTERUPT_PIN 2

void setup() {
  
  Serial.begin(115200);  // initialize Serial interface
  while (!Serial) 
  {
    delay(200); // wait for serial port to connect. Needed for native USB
  }
  
  Serial.println("Starting...");

  attachInterrupt(digitalPinToInterrupt(INTERUPT_PIN), onPulse2, FALLING);
}

void loop() {
  Serial.println(".");
  delay(200);

}

void onPulse2()
{
  if (digitalRead(INTERUPT_PIN)) {
    Serial.println("Interupt 2 triggered...");
  }
}
