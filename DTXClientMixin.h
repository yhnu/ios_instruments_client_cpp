#pragma once
#include <libimobiledevice/ext.h>
#include "DTXFragment.h"
#include "DTXUSBTransport.h"
#include "dxtmsg.h"

template <typename T>
struct array_deleter
{
    void operator()(T const* p) { delete[] p; }
};

#define DTXClientMixin_t std::shared_ptr<DTXClientMixin>
class DTXClientMixin : public DTXUSBTransport
{
public:
    bool send_dtx(instrument_client_t iclient, DTXMessage& dtx)
    {
        OutBuffer out;
        dtx.to_bytes(out);

        auto bsucceed = this->send_all(iclient, (uint8_t*)out.buffer(), out.length());
        return bsucceed;
    }

    DTXMessage_t recv_dtx(instrument_client_t iclient)
    {
        while (true)
        {
            OutBuffer out;
            if (!recv_dtx_fragment(iclient, out))
            {
                SPDLOG_ERROR("recv_dtx_fragment failed");
                return nullptr;
            }
            SPDLOG_INFO("out={} len={}", out.buffer(), out.length());
            DTXFragment fragment(out);
            if (fragment.completed())
            {
                return fragment.meessage();
            }
            else
            {
                SPDLOG_ERROR("! fragment.completed()");
                return nullptr;
            }
        }
    }

private:
    bool recv_dtx_fragment(instrument_client_t iclient, OutBuffer& out)
    {
        int headerSize                                  = sizeof(DTXMessageHeader);
        std::shared_ptr<DTXMessageHeader> header_buffer = std::make_shared<DTXMessageHeader>();
        // DTXMessageHeader* header_buffer                 = (DTXMessageHeader*)malloc(headerSize);

        if (!this->recv_all(iclient, (uint8_t*)header_buffer.get(), headerSize))
        {
            SPDLOG_ERROR("recv_all error");
            return false;
        }

        if (header_buffer->fragmentId == 0 && header_buffer->fragmentCount > 1)
        {
            out.write(*header_buffer);
            return true;
        }

        int bodySize = header_buffer->length;
        std::shared_ptr<uint8_t> body_buffer(new uint8_t[bodySize], array_deleter<uint8_t>());

        if (!this->recv_all(iclient, body_buffer.get(), bodySize))
        {
            SPDLOG_ERROR("recv_all error");
            return false;
        }
        else
        {
            out.write(*header_buffer);
            out.write(body_buffer.get(), bodySize);
            return true;
        }
    }
};
