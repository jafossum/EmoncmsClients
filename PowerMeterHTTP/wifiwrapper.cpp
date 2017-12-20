#include "wifiwrapper.h"

WiFiWrapper::WiFiWrapper(){}

void WiFiWrapper::init() {

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while (!this->isConnected()) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("Your ESP is connected!");  
    Serial.print("Your IP address is: ");
    Serial.println(WiFi.localIP());
}

boolean WiFiWrapper::isConnected() {
    // check our Wifi is still ok
    return (WiFi.status() == WL_CONNECTED);
}