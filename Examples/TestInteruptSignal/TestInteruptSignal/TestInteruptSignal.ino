
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

//----- ISR variables ---------
volatile unsigned long minElapsed = 50;
volatile unsigned long elapsedTime, previousTime;

void setup() {
  
  Serial.begin(115200);  // initialize Serial interface
  while (!Serial) 
  {
    delay(200); // wait for serial port to connect. Needed for native USB
  }
  
  Serial.println("Starting...");
  Serial.println(D4);

  attachInterrupt(digitalPinToInterrupt(D4), onPulse2, FALLING);
}

void loop() {
  delay(500);
}

void onPulse2()
{
  elapsedTime = millis() - previousTime;
  Serial.println(elapsedTime);
  
  if (elapsedTime < minElapsed)  //false interrupt
  {
    return;
  }
  if (elapsedTime >= minElapsed)  //in range
  {
    previousTime = millis();
    {
      Serial.println("Interupt 2 triggered...");
    }
  }
}
