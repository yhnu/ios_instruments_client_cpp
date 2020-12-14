#pragma once
#include "dxtmsg.h"

class DTXFragment
{
public:
    DTXFragment(OutBuffer& buf)
    {
        _buf.write(buf.buffer(), buf.length());
        _header = DTXMessageHeader::from_buffer_copy((uint8_t*)buf.buffer(), 0, sizeof(DTXMessageHeader));
        if (_header->fragmentId == 0)
        {
            current_fragment_id = 0;
        }
        else
        {
            current_fragment_id = -1;
        }
    }
    DTXMessage_t meessage() { return DTXMessage::from_bytes((uint8_t*)_buf.buffer(), _buf.length()); }
    bool completed() { return current_fragment_id + 1 == _header->fragmentCount; }
    bool header() { return _header->fragmentId == 0; }

private:
    DTXMessageHeader_t _header;
    int current_fragment_id;
    OutBuffer _buf;
};
