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

#define REPORTING_INTERVAL_MS  2500

// Comment this out for not printing data to the serialport
#define DEBUG

// Wifi data
const char* ssid = "Fossum";
const char* password = WIFI_PASSWORD; // PASSWORD from Secrets.h

// HTTP Client
WiFiClient client;

//Emoncms configurations
char server[] = "emoncms.org";     // name address for emoncms.org
String apikey = API_KEY;  // API_KEY from Secrets.h
int node = 0; //if 0, not used

typedef struct { int power, pulse;} PayloadTX;
PayloadTX emontx; 

// Timer used for timing callbacks
Timer callback_timer;                             

// ----------- Pinout assignments  -----------
//
// digital input pins:
// dig pin 1 for input signal

const byte INT_PIN = 1;

// Pulse counting settings 
long pulseCount = 0;               // Number of pulses, used to measure energy.
unsigned long pulseTime,lastTime;  // Used to measure power.
double power, elapsedWh;           // power and energy
int ppwh = 1;                      // pulses per watt hour

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

  // Attach interupt for cpunting light pulses on powercentral
  
  attachInterrupt(digitalPinToInterrupt(INT_PIN), onPulse, FALLING);
  // attachInterrupt(INT_PIN, onPulse, FALLING);

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

  emontx.pulse = pulseCount; pulseCount=0;

  publishData(&emontx.power, &emontx.pulse);

#ifdef DEBUG
  Serial.print("W: ");
  Serial.println(emontx.power);
  Serial.print("Pulse: ");
  Serial.println(emontx.pulse);
#endif
}

void publishData(int *power, int *ppulse)
{
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("Connecting...");
    
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
  lastTime = pulseTime;        //used to measure time between pulses.
  pulseTime = micros();
  pulseCount++;                                                      //pulseCounter               
  emontx.power = int((3600000000.0 / (pulseTime - lastTime))/ppwh);  //Calculate power

#ifdef DEBUG
  Serial.print("Power: ");
  Serial.print(emontx.power);
  Serial.println(" W");
#endif
}
