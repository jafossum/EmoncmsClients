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

#define REPORTING_INTERVAL_MS 5000

// Comment this out for not printing data to the serialport after Setup()
#define DEBUG

// EmonCms Client
EmonCms client;

// Wifi Client
WiFiWrapper wifi;

// ----------- Pinout assignments  -----------
//
// digital input pins:
// dig pin D1 for input signal 

// Pulse counting settings 
int pulseCount = 0;                // Number of pulses, used to measure energy.
int power[50] = { };               // Array to store pulse power values
long txpower = 0;                  // powernumber to send
int txpulse = 0;                   // number of pulses to send
unsigned long pulseTime,lastTime;  // Used to measure power.
int ppwh = 1;                      // pulses per watt hour
long _sum = 0;                     // Helper for calculating average
int _pulsecount = 0;               // Helper for calcualting average

//----- Interupt filtering variables ---------
volatile unsigned long minElapsed = 100;
volatile unsigned long elapsedTime, previousTime;

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

  // enable the watchdog timer - 8s timeout
  ESP.wdtEnable(8000);
}

void loop()             
{
  delay(REPORTING_INTERVAL_MS);

  // reset the watchdog timer
  ESP.wdtFeed();

  // check our Wifi is still ok - Watchdog timer will catch this if not connected
  while (!wifi.isConnected()) {
      delay(500);
  #ifdef DEBUG
      Serial.println("WiFi not connected!");
  #endif
  }

  send_data();
}

void send_data()
{  
  // Calculate average over the last power meassurements before sending
  _sum = 0;
  _pulsecount = pulseCount;
  
  for(int i=1; i<=_pulsecount; i++) {
    _sum += power[i];
  }

  txpower = int(_sum / _pulsecount);
  txpulse = _pulsecount;
   
  pulseCount=0;
  power[50] = { };

  client.publishData(&txpower, &txpulse);

#ifdef DEBUG
  Serial.print("W: ");
  Serial.print(txpower);
  Serial.print(" - Pulse: ");
  Serial.println(txpulse);
#endif
}

// The interrupt routine - runs each time a falling edge of a pulse is detected
void onPulse()                  
{
  elapsedTime = millis() - previousTime;

  if (elapsedTime >= minElapsed)  //in range
  {
    previousTime = millis();
    
    lastTime = pulseTime;        //used to measure time between pulses.
    pulseTime = micros();

    // Increase pulseCounter
    pulseCount++;
    
    // Size of array to avoid runtime error
    if (pulseCount < 50) {
      power[pulseCount] = int((3600000000.0 / (pulseTime - lastTime))/ppwh);  //Calculate power
      
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
