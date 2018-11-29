#include "application.h"
#include "emoncms.h"

EmonCms::EmonCms(){}

void EmonCms::init() {
}

void EmonCms::publishData(long *power, int *ppulse)
{
    // send the webhook
    String data = "{pulsecount:" + String(*power) + ",power:" + String(*ppulse) + "}";
    Particle.publish("powermeter", data, 10, PRIVATE);
}