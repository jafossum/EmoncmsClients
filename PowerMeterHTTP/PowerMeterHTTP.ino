/*
  Arduino & OpenEnergyMonitor 
  
  This sketch is based on this example
  https://github.com/OfficineArduino/Tutorial-Wired/blob/master/OpenEnergyMonitor/emon_simple_client/emon_simple_client.ino
  
  Sketch is modified to use a NodeMCU - ESP8622 to connect 
  to an emoncms server and makes a request
*/

#include "Arduino.h"
#include "emoncms.h"
#include "wifiwrapper.h"
#include "Secrets.h"

// Comment this out for not printing data to the serialport after Setup()
#define DEBUG
#define REPORTING_INTERVAL_MS 10000     // Reporting interval (ms)
#define MIN_ELAPSED_TIME 100000         // Filtering min elapsed time (microSec)
#define PPWH 2                          // pulses per watt hour

// EmonCms Client
EmonCms client;

// Wifi Client
WiFiWrapper wifi;

// ----------- Pinout assignments  -----------
//
// digital input pins:
// dig pin D1 for input signal 

// Pulse counting settings 
volatile int pulseCount = 0;       // Number of pulses, used to measure energy.
volatile int power[50] = { };      // Array to store pulse power values
unsigned long pulseTime = 0;       // Used to measure time between pulses.
unsigned long previousTime = 0;       // Main loop timing variable

void setup()
{  
  // ensure the watchdog is disabled
  ESP.wdtDisable();

  Serial.begin(115200);  // initialize Serial interface
  while (!Serial) 
  {
    delay(200); // wait for serial port to connect. Needed for native USB
  }

  Serial.println("-------------------------------------");
  Serial.println("Sketch ID:      PowerMeterHTTP.ino");
  Serial.println ("Getting IP address...");
  Serial.print("Your are connecting to: ");
  Serial.println(WIFI_SSID);

  // Setup WiFi
  wifi.init();

  // Set up EmonCms Client
  client.init();

  // Attach interupt for capturing light pulses on powercentral
  attachInterrupt(digitalPinToInterrupt(D1), onPulse, FALLING);

  // enable the watchdog timer - 4s timeout
  ESP.wdtEnable(WDTO_4S);
}

void loop()             
{
  // reset the watchdog timer
  ESP.wdtFeed();

  unsigned long elapsedTime = millis() - previousTime;
  if (elapsedTime >= REPORTING_INTERVAL_MS) 
  {
    previousTime = millis();

    verifyWifiConnected();
    send_data();
  }
}

// If wifi not connected this will not return. Causes Watchdog timeout.
void verifyWifiConnected() {
  // check Wifi is still ok - Watchdog timer will catch this if not connected
  while (!wifi.isConnected()) {
    Serial.println("WiFi not connected!");
    delay(500);
  }
}

// Periodic calculate and send data method
void send_data()
{  
  // Calculate average over the last power meassurements before sending
  int _pulsecount = pulseCount;      // Helper for calcualting average. Not using pulseCount because of interupt update
  long _sum = 0;                     // Helper for calculating average

  if (_pulsecount > 0) {

    for(int i=1; i<=_pulsecount; i++) {
      _sum += power[i];
    }
    pulseCount=0;
    power[50] = { };

    long txpower = (_sum / _pulsecount);

    if (txpower > 0 && txpower < 20000) {
      client.publishData(&txpower, &_pulsecount);
    }

#ifdef DEBUG
    Serial.print("W: ");
    Serial.print(txpower);
    Serial.print(" - Pulse: ");
    Serial.println(_pulsecount);
#endif
  }
}

// The interrupt routine - runs each time a falling edge of a pulse is detected
void onPulse()                  
{
  unsigned long elapsedTime = micros() - pulseTime;
  if (elapsedTime >= MIN_ELAPSED_TIME)  //in range
  {      
    pulseTime = micros();       //used to measure time between pulses.

    // Increase pulseCounter
    pulseCount++;
    
    // Size of array to avoid runtime error
    if (pulseCount < 50) {
      power[pulseCount] = int((3600000000.0 / elapsedTime) / PPWH);  //Calculate power
      
#ifdef DEBUG
      Serial.print("Power: ");
      Serial.print(power[pulseCount]);
      Serial.print(" W - Count: ");
      Serial.println(pulseCount);
#endif
    }
    else {
      Serial.println("Pulsecount over 50. Not logging....");
    }
  }
}
