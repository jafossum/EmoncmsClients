/*
  Arduino & OpenEnergyMonitor 
  
  This sketch is based on this example
  https://github.com/OfficineArduino/Tutorial-Wired/blob/master/OpenEnergyMonitor/emon_simple_client/emon_simple_client.ino
  
  Sketch is modified to use a NodeMCU - ESP8622 to connect 
  to an emoncms server and makes a request
*/

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include <Timer.h>
#include <ESP8266WiFi.h>
#include "Secrets.h"

#define REPORTING_INTERVAL_MS  5000

// Comment this out for not printing data to the serialport
#define DEBUG

// Wifi data
const char* ssid = WIFI_SSID;  // SSID from Secrets.h
const char* password = WIFI_PASSWORD;  // PASSWORD from Secrets.h

// HTTP Client
WiFiClient client;

//Emoncms configurations
char server[] = "emoncms.org";     // name address for emoncms.org
String apikey = API_KEY;  // API_KEY from Secrets.h
int node = 0; //if 0, not used

// Timer used for timing callbacks
Timer callback_timer;                             

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

#ifdef DEBUG
  Serial.println("-------------------------------------");
  Serial.println("Sketch ID:      PowerMeterHTTP.ino");
  Serial.println ("Getting IP address...");
  Serial.print("Your are connecting to: ");
  Serial.println(ssid);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
  }

#ifdef DEBUG
  Serial.println("");
  Serial.println("Your ESP is connected!");  
  Serial.print("Your IP address is: ");
  Serial.println(WiFi.localIP());
#endif

  // Setup for report event timing
  int reportEvent = callback_timer.every(REPORTING_INTERVAL_MS, send_data);

  // Attach interupt for capturing light pulses on powercentral
  attachInterrupt(digitalPinToInterrupt(D1), onPulse, FALLING);

  // enable the watchdog timer - 6s timeout
  ESP.wdtEnable(6000);
}

void loop()             
{ 
  callback_timer.update();
}

void send_data()
{
  // reset the watchdog timer
  ESP.wdtFeed();
  
  // check our Wifi is still ok - Watchdog timer will catch this if not connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.println("WiFi not connected!");
#endif
  }

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

  publishData(&txpower, &txpulse);

#ifdef DEBUG
  Serial.print("W: ");
  Serial.print(txpower);
  Serial.print(" - Pulse: ");
  Serial.println(txpulse);
#endif
}

void publishData(long *power, int *ppulse)
{
  // if there's a successful connection:
  if (client.connect(server, 80)) {
#ifdef DEBUG
    Serial.println("Connecting...");
#endif
    
    // send the HTTP GET request:
    client.print("GET /api/post?apikey=");
    client.print(apikey);
    
    if (node > 0) {
      client.print("&node=");
      client.print(node);
    }
    client.print("&json={pulsecount:");
    client.print(*ppulse);    
    client.print(",power:");
    client.print(*power);   
    client.println("} HTTP/1.1");
    client.println("Host:emoncms.org");
    client.println("User-Agent: ESP8266");
    client.println("Connection: close");
    client.println();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("Connection failed");
    Serial.println("Disconnecting...");
    client.stop();
  }

  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
#ifdef DEBUG
  delay(500);
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
  Serial.println();
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
