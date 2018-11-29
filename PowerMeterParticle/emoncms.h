#ifndef emoncms_h
#define emoncms_h

class EmonCms
{
public:
    EmonCms();
    void init();
    void publishData(long *power, int *ppulse);
};

#endif