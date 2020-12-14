#include <assert.h>
#include <memory.h>
#include <cstdio>
#include <cstdlib>
#include <iomanip>  // std::setw
#include <iostream>
#include <vector>
#include "spdlog/spdlog.h"
#pragma once

class InBuffer
{
    uint8_t* _buf;
    uint32_t _limit;
    uint32_t _cur;

public:
    InBuffer(uint8_t* buf, uint32_t len) : _cur(0), _buf(buf), _limit(len) {}

    template <typename T>
    int read(T& ret)
    {
        if (_cur + sizeof(T) > _limit)
            return 0;
        T* _ret = (T*)(_buf + _cur);
        _cur += sizeof(T);
        ret = *_ret;
        return sizeof(T);
    }

    const void* read_buf(int len)
    {
        if (_cur + len > _limit)
        {
            return NULL;
        }
        void* ret = (void*)(_buf + _cur);
        _cur += len;
        return ret;
    }

    int last_len() { return _limit - _cur; }
    int get_cursor() { return _cur; }
};

class OutBuffer
{
public:
    OutBuffer(int cap = 0) : capacity(cap), buf(nullptr), len(0)
    {
        if (cap != 0)
        {
            buf = malloc(cap);
        }
    }

    ~OutBuffer()
    {
        if (buf)
        {
            free(buf);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const OutBuffer& rhs)
    {
        os << "|buf: " << std::hex << rhs.buf << "|";
        return os;
    }

    void* buffer() { return buf; }

    size_t length() { return len; }

    template <typename T>
    void write(const T& t)
    {
        while (len + sizeof(t) > capacity)
        {
            expand();
        }
        *(T*)((uint8_t*)buf + len) = t;
        len += sizeof(t);
    }

    void write(const void* b, size_t l)
    {
        while (len + l > capacity)
        {
            expand();
        }
        memcpy((uint8_t*)buf + len, b, l);
        len += l;
    }

    void reset() { len = 0; }

    void reset(size_t new_pos)
    {
        assert(new_pos <= len);
        len = new_pos;
    }

    __attribute__((always_inline)) void expand()
    {
        // LOGI("EXPAND: cap=%zu len=%zu", capacity, len);
        size_t new_cap = capacity * 2;
        if (new_cap == 0)
        {
            new_cap = 4096 - 8;
        }
        void* new_buf = malloc(new_cap);
        if (buf)
        {
            memcpy(new_buf, buf, len);
            free(buf);
        }

        buf      = new_buf;
        capacity = new_cap;
    }

private:
    void* buf;
    size_t capacity;
    size_t len;
};

#define DTXMessageHeader_t std::shared_ptr<DTXMessageHeader>

class DTXMessageHeader
{
public:
    uint32_t magic;
    uint32_t cb;
    uint16_t fragmentId;
    uint16_t fragmentCount;
    uint32_t length;
    uint32_t identifier;
    uint32_t conversationIndex;
    uint32_t channelCode;
    uint32_t expectsReply;

public:
    DTXMessageHeader()
    {
        this->magic         = 0x1f3d5b79;
        this->cb            = 0x20;
        this->fragmentId    = 0;
        this->fragmentCount = 1;
    }
    static DTXMessageHeader_t from_buffer_copy(uint8_t* buf, int offset, int size)
    {
        DTXMessageHeader_t p = std::make_shared<DTXMessageHeader>();
        memcpy(p.get(), buf + offset, size);
        return p;
    }

    friend std::ostream& operator<<(std::ostream& os, const DTXMessageHeader& dtx_meesage_header)
    {
        os << "DTXMessageHeader(";
        os << "magic:0x" << std::hex << dtx_meesage_header.magic << "|";
        os << "cb:0x" << std::hex << dtx_meesage_header.cb << "|";
        os << "fragmentId:0x" << dtx_meesage_header.fragmentId << "|";
        os << "fragmentCount:0x" << dtx_meesage_header.fragmentCount << "|";
        os << "length:0x" << dtx_meesage_header.length << "|";
        os << "identifier:0x" << dtx_meesage_header.identifier << "|";
        os << "conversationIndex:0x" << dtx_meesage_header.conversationIndex << "|";
        os << "channelCode:0x" << dtx_meesage_header.channelCode << "|";
        os << "expectsReply:0x" << dtx_meesage_header.expectsReply << ")";
        return os;
    }
};

class DTXPayloadHeader
{
public:
    uint32_t flags;
    uint32_t auxiliaryLength;
    uint64_t totalLength;

    DTXPayloadHeader()
    {
        flags           = 0x2;
        auxiliaryLength = 0x0;
        totalLength     = 0x0;
    }

    friend std::ostream& operator<<(std::ostream& os, const DTXPayloadHeader& rhs)
    {
        os << "DTXPayloadHeader(";
        os << "flags: 0x" << std::hex << rhs.flags << "|";
        os << "auxiliaryLength: 0x" << std::hex << rhs.auxiliaryLength << "|";
        os << "totalLength: 0x" << rhs.totalLength << ")";
        return os;
    }
};

// helper class for serializing method arguments
// the final serialized array must start with a magic qword,
// followed by the total length of the array data as a qword,
// followed by the array data itself.
class DTXAuxiliariesHeader
{
public:
    u_int64_t magic;
    int64_t length;

    DTXAuxiliariesHeader()
    {
        magic  = 0x1f0;
        length = 0;
    }

    friend std::ostream& operator<<(std::ostream& os, const DTXAuxiliariesHeader& rhs)
    {
        os << "DTXAuxiliariesHeader(";
        os << "magic: 0x" << std::hex << rhs.magic << "|";
        os << "length: 0x" << std::hex << rhs.length << ")";
        return os;
    }
};

class DTXAuxiliary
{
public:
    int32_t magic;
    int32_t type;
    int32_t obj_len;
    const void* obj;

public:
    DTXAuxiliary()
    {
        obj   = nullptr;
        magic = 0xa;
        type  = -1;
    }

    void serial(OutBuffer& out)
    {
        if (obj == nullptr)
        {
            SPDLOG_ERROR("obj == nullptr");
            exit(1);
        }
        out.write(magic);
        out.write(type);
        if (type == 2)
            out.write(obj_len);
        out.write(obj, obj_len);
    }

    friend std::ostream& operator<<(std::ostream& os, const DTXAuxiliary& rhs)
    {
        os << "DTXAuxiliary(";
        os << "magic: 0x" << std::hex << rhs.magic << "|";
        os << "type: 0x" << std::hex << rhs.type << "|";
        if (rhs.obj != nullptr)
        {
            std::shared_ptr<char> buf(new char[1024]);
            unsigned char* chunk = (unsigned char*)rhs.obj;
            int len              = rhs.obj_len > 5 ? 5 : rhs.obj_len;
            for (int i = 0; i < len; i++)
            {
                char b[10];
                sprintf(b, " %02x", chunk[i]);
                strcat(buf.get(), b);
            }
            os << "obj: " << buf << "|";
        }
        else
        {
            os << "obj: null|";
        }
        return os;
    }
};

class DTXSelector
{
public:
    void* buf;
    int len;
    DTXSelector() : buf(nullptr), len(0) {}
    void serial(OutBuffer& out) { out.write(buf, len); }
    friend std::ostream& operator<<(std::ostream& os, const DTXSelector& rhs)
    {
        os << "DTXSelector(";
        if (rhs.buf != nullptr)
        {
            char* p = (char*)rhs.buf;
            for (int i = 0; i < rhs.len; i++)
            {
                if (!(p[i] == '\0' || p[i] == '\n' || p[i] == ' '))
                {
                    os << p[i];
                }
            }
        }
        else
        {
            os << "obj: null|";
        }
        return os;
    }
};

#define DTXMessage_t std::shared_ptr<DTXMessage>

class DTXMessage
{
public:
    uint8_t* _buf;
    int _buf_size;

    DTXMessageHeader _message_header;
    DTXPayloadHeader _payload_header;
    DTXAuxiliariesHeader _auxiliaries_header;
    std::vector<DTXAuxiliary> _auxiliaries;
    DTXSelector _selector;

public:
    DTXMessage() : _buf(nullptr), _buf_size(0) {}
    ~DTXMessage()
    {
        if (_buf)
        {
            free(_buf);
            _buf_size = 0;
        }
    }

    void Init(uint8_t* buf, int len)
    {
        if (buf != nullptr && len > 0)
        {
            resize_buf(len);
            memcpy(this->_buf, buf, len);
        }
    }

protected:
    void resize_buf(int new_size)
    {
        if (_buf && new_size > _buf_size)
        {
            _buf      = (uint8_t*)realloc(_buf, new_size);
            _buf_size = new_size;
        }
        else if (_buf == nullptr)
        {
            _buf      = (uint8_t*)malloc(new_size);
            _buf_size = new_size;
        }
        else
        {
        }
    }

public:
    friend std::ostream& operator<<(std::ostream& os, const DTXMessage& rhs)
    {
        os << "DTXMessage(\n";
        os << "\t" << rhs._message_header << std::endl;
        os << "\t" << rhs._payload_header << std::endl;
        os << "\t" << rhs._auxiliaries_header << std::endl;
        for (auto& e : rhs._auxiliaries)
        {
            os << "\t" << e << std::endl;
        }
        os << "\t" << rhs._selector << std::endl;

        return os;
    }

public:
    static DTXMessage_t from_bytes(uint8_t* buf, const int len)
    {
        DTXMessage_t dtx = std::make_shared<DTXMessage>();
        dtx->Init(buf, len);

        InBuffer in(dtx->_buf, dtx->_buf_size);
        spdlog::info("{} {}", dtx->_buf, dtx->_buf_size);

        in.read<DTXMessageHeader>(dtx->_message_header);
        std::cout << dtx->_message_header << std::endl;

        bool has_payload = dtx->_message_header.length > 0;

        if (!has_payload)
        {
            SPDLOG_WARN("has no payload");
            return dtx;
        }

        if (dtx->_message_header.length != dtx->_buf_size - dtx->_message_header.fragmentCount * sizeof(DTXMessageHeader))
        {
            SPDLOG_ERROR("[CRASH]incorrect DTXMessageHeader->length");
            exit(1);
        }
        else
        {
            if (dtx->_message_header.fragmentCount != 1)
            {
                SPDLOG_ERROR("[CRASH]not impl");
                exit(1);
            }

            in.read<DTXPayloadHeader>(dtx->_payload_header);
            std::cout << dtx->_payload_header << std::endl;

            if (dtx->_payload_header.totalLength == 0)
            {
                SPDLOG_INFO("_payload_header.totalLength == 0");
                return dtx;
            }
            else if (dtx->_payload_header.totalLength != in.last_len())
            {
                SPDLOG_ERROR("incorrect DTXPayloadHeader->totalLength");
                exit(1);
            }

            if (dtx->_payload_header.auxiliaryLength > 0)
            {
                in.read<DTXAuxiliariesHeader>(dtx->_auxiliaries_header);
                std::cout << dtx->_auxiliaries_header << std::endl;
                int aux_length = 0;
                while (aux_length < dtx->_auxiliaries_header.length)
                {
                    int len = 0;
                    DTXAuxiliary aux;
                    len += in.read(aux.magic);
                    len += in.read(aux.type);
                    SPDLOG_INFO("{:x} {:x} {:x}", in.get_cursor(), aux.magic, aux.type);

                    if (aux.magic != 0xa)
                    {
                        SPDLOG_ERROR("incorrect auxiliary magic");
                        exit(1);
                    }
                    if (aux.type == 2)
                    {
                        // 4byte
                        len += in.read<int32_t>(aux.obj_len);
                        aux.obj = in.read_buf(aux.obj_len);
                        SPDLOG_INFO("l =  {:x}, cur={:x}", aux.obj_len, in.get_cursor());

                        len += aux.obj_len;
                    }
                    else if (aux.type == 4)  // || aux.type == 3)
                    {
                        // 4byte * aux.type
                        aux.obj_len = 16 - 8;
                        aux.obj     = in.read_buf(aux.obj_len);
                        len += aux.obj_len;
                    }
                    else if (aux.type == 3)
                    {
                        aux.obj_len = 12 - 8;
                        aux.obj     = in.read_buf(aux.obj_len);
                        len += aux.obj_len;
                    }
                    else
                    {
                        SPDLOG_ERROR("unknown auxiliary type");
                        exit(1);
                    }
                    aux_length += len;
                    dtx->_auxiliaries.push_back(aux);
                }
                if (aux_length != dtx->_auxiliaries_header.length)
                {
                    SPDLOG_ERROR("incorrect DTXAuxiliariesHeader.length");
                    exit(1);
                }
                else
                {
                    SPDLOG_INFO("aux_length={}, _auxiliaries_header.length={}", aux_length, dtx->_auxiliaries_header.length);
                }
            }
            dtx->_selector.len = in.last_len();
            dtx->_selector.buf = (char*)in.read_buf(dtx->_selector.len);
        }
        return dtx;
    }

    bool to_bytes(OutBuffer& out)
    {
        OutBuffer payload_buf;
        payload_buf.write(this->_payload_header);
        payload_buf.write(this->_auxiliaries_header);

        SPDLOG_INFO("size={}", this->_auxiliaries.size());
        for (int i = 0; i < this->_auxiliaries.size(); i++)
        {
            this->_auxiliaries[i].serial(payload_buf);
        }
        this->_selector.serial(payload_buf);

        if (payload_buf.length() > 65504)
        {
            SPDLOG_ERROR("payload_buf.length() > 65504");
            exit(1);
        }
        else
        {
            out.write(this->_message_header);
            out.write(payload_buf.buffer(), payload_buf.length());
        }
        return true;
    }
};

int div_ceil(int p, int q);
int div_floor(int p, int q);
