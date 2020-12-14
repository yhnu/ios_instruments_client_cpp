
#include <libimobiledevice/libimobiledevice.h>
#pragma once

class instrument_rpc
{
private:
    idevice_t client;

public:
    instrument_rpc(/* args */);
    ~instrument_rpc();
};
