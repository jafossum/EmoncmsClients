#ifndef wifiwrapper_h
#define wifiwrapper_h

#include <ESP8266WiFi.h>
#include "Secrets.h"

class WiFiWrapper
{
private:

public:
    WiFiWrapper();
    void init();
    boolean isConnected();
};

#endif