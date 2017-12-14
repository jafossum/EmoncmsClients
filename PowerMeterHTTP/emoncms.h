#ifndef emoncms_h
#define emoncms_h

#include <WiFiClientSecure.h>
#include "Secrets.h"

class EmonCms
{
private:
    //Emoncms configurations
    const char* server = "emoncms.org";     // name address for emoncms.org
    const int httpsPort = 443;
    const int node = 0;                         //if 0, not used
    // SHA1 fingerprint of the certificate
    const char* fingerprint = "B4 58 91 74 C9 33 52 18 1A A5 A1 81 32 60 9D CB 6E 69 53 C2";
    // HTTPS Client
    WiFiClientSecure secureClient;

public:
    EmonCms();
    void init();
    void publishData(long *power, int *ppulse);
};

#endif