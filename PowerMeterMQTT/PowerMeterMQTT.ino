//      Code by Robin Emley (calypso_rae on Open Energy Monitor Forum) - September 2013
//      Updated November 2013 to include analog and LED pins for the emonTx V3 by Glyn Hudson
//
//      Updated July 2014 to send readings via MQTT by Ben Jones
//
//      The interrupt-based kernel for this sketch was kindly provided by Jorg Becker.
//
//      Copied 2017 from Thomas Bergo and adapted as needed

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif
#include <avr/wdt.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Timer.h>

#define REPORTING_INTERVAL_MS  5000
#define ENVIRONMENT_INTERVAL_MS 60000
#define CYCLES_PER_SECOND 50

// Comment this out for not printing data to the serialport
#define DEBUG

// unique MAC address on our LAN (0x53 => .83)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x53 };

// MQTT broker connection properties
byte mqttBroker[] = { 192, 168, 1, 170 };
char mqttClientId[] = "emontx";
//char mqttUsername[] = "USERNAME";
//char mqttPassword[] = "PASSWORD";

// publish to "/emontx/<variable>".
char mqttTopic[] = "/emontx";
char mqttTopicLwt[] = "/clients/emontx";
int  mqttLwtQos = 0;
int  mqttLwtRetain = 1;

// ethernet shield uses pins 4 for SD card and 10-13 for SPI
EthernetClient ethernet;

//PubSubClient mqtt(mqttBroker, 1883, mqtt_callback, ethernet);
PubSubClient mqtt(mqttBroker, 1883, 0, ethernet);
typedef struct { int power, pulse;} PayloadTX;
PayloadTX emontx; 

// Timer used for timing callbacks
Timer t;                             

// ----------- Pinout assignments  -----------
//
// digital input pins:
// dig pin 0 is for Serial Rx
// dig pin 1 is for Serial Tx
// dig pin 4 is for the ethernet module (IRQ) 
// dig pin 10 is for the ethernet module (SEL) 
// dig pin 11 is for the ethernet module (SDI) 
// dig pin 12 is for the ethernet module (SDO) 
// dig pin 13 is for the ethernet module (CLK) 
const byte INT_PIN = 1;

// Pulse counting settings 
long pulseCount = 0;                                                    // Number of pulses, used to measure energy.
unsigned long pulseTime,lastTime;                                       // Used to measure power.
double power, elapsedWh;                                                // power and energy
int ppwh = 1;

void mqtt_callback(char *topic, byte *payload, unsigned int length) 
{
  // no incoming messages to process
}

void setup()
{  
  // ensure the watchdog is disabled
  wdt_disable();

#ifdef DEBUG
  Serial.begin(9600);                                      // initialize Serial interface
  while (!Serial) 
  {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("-------------------------------------");
  Serial.println("Sketch ID:      PowerMeterMQTT.ino");
#endif

  // initialise the SPI bus.  
  SPI.begin();

  // get an IP address
#ifdef DEBUG
  Serial.println ("Getting IP address...");
#endif

  while (Ethernet.begin(mac) != 1) {
    delay(5000);
  }

  // connect to our MQTT broker
#ifdef DEBUG
  Serial.println ("Connect to MQTT broker...");
#endif
  while (!mqttConnect()) {
    delay(5000);
  }

  int reportEvent = t.every(REPORTING_INTERVAL_MS, send_data);

  attachInterrupt(INT_PIN, onPulse, FALLING);

  // enable the watchdog timer - 8s timeout
  wdt_enable(WDTO_8S);
  wdt_reset();
}


void loop()             
{ 
  // reset the watchdog timer
  wdt_reset();
  t.update();
  mqtt.loop();
}

boolean mqttConnect() 
{
  //boolean success = mqtt.connect(mqttClientId, mqttUsername, mqttPassword, mqttTopicLwt, mqttLwtQos, mqttLwtRetain, "0"); 
  boolean success = mqtt.connect(mqttClientId, mqttTopicLwt, mqttLwtQos, mqttLwtRetain, "0");
  
  if (success) 
  {
#ifdef DEBUG
    Serial.println ("Successfully connected to MQTT broker");
#endif
    // publish retained LWT so anything listening knows we are alive
    byte data[] = { "1" };
    mqtt.publish(mqttTopicLwt, data, 1, mqttLwtRetain);
  } 
#ifdef DEBUG
  else 
  {
    Serial.println ("Failed to connect to MQTT broker");
  }
#endif
  return success;
}

void send_data()
{
  // check our DHCP lease is still ok
  Ethernet.maintain();

  // process any MQTT messages - will return false if not connected
  if (!mqtt.loop()) 
  {
    // keep trying to connect - watchdog timer will fire if we can't
    while (!mqttConnect()) 
    {
      delay(2000);
    }
  }

  emontx.pulse = pulseCount; pulseCount=0;

  char power[6] = "power";
  char pls[6] = "pulse";
  publishData(power, &emontx.power);
  publishData(pls, &emontx.pulse);

#ifdef DEBUG
  Serial.print(emontx.power);
  Serial.print("W: ");
  Serial.print(emontx.pulse);
  Serial.println("Pulse ");
#endif
}

void publishData(char *name, int *value)
{
  // build the MQTT topic
  char topic[32];
  snprintf(topic, 32, "%s/%s", mqttTopic, name);

  // build the MQTT payload
  char payload[16];
  snprintf(payload, 16, "%i", value);

  // publish to the MQTT broker 
  mqtt.publish(topic, payload);

#ifdef DEBUG
  //Serial.println(topic);
  //Serial.println(payload);
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
  //Serial.println("Pulse");
#endif
}
