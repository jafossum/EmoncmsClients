#include "emoncms.h"

EmonCms::EmonCms(){}

void EmonCms::init() {
  // Use WiFiClientSecure class to create TLS connection
  Serial.print("connecting to ");
  Serial.println(EmonCms::server);
  if (!EmonCms::secureClient.connect(EmonCms::server, EmonCms::httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  // Optinal - Check SHA-1 fingerprint (Changes every 2-3 months)
  if (EmonCms::secureClient.verify(EmonCms::fingerprint, EmonCms::server)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }
}

void EmonCms::publishData(long *power, int *ppulse)
{
  // if there's a successful connection:
  if (EmonCms::secureClient.connect(EmonCms::server, EmonCms::httpsPort)) {
#ifdef DEBUG
    Serial.println("Connecting...");
#endif
    
    // send the HTTP GET request:
    EmonCms::secureClient.print("GET /api/post?apikey=");
    EmonCms::secureClient.print(API_KEY);
    
    if (node > 0) {
        EmonCms::secureClient.print("&node=");
        EmonCms::secureClient.print(node);
    }
    EmonCms::secureClient.print("&json={pulsecount:");
    EmonCms::secureClient.print(*ppulse);    
    EmonCms::secureClient.print(",power:");
    EmonCms::secureClient.print(*power);   
    EmonCms::secureClient.println("} HTTP/1.1");
    EmonCms::secureClient.println("Host:emoncms.org");
    EmonCms::secureClient.println("User-Agent: ESP8266");
    EmonCms::secureClient.println("Connection: close");
    EmonCms::secureClient.println();
  }
  else {
    // if you couldn't make a connection:
    Serial.println("Connection failed");
    Serial.println("Disconnecting...");
    EmonCms::secureClient.stop();
  }

  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
#ifdef DEBUG
  delay(500);
  while (EmonCms::secureClient.available()) {
    char c = EmonCms::secureClient.read();
    Serial.print(c);
  }
  Serial.println();
#endif

}         

