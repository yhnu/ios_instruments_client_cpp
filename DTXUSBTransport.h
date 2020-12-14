// """
// Instruments 服务，用于监控设备状态, 采集性能数据
// """
#include <libimobiledevice/ext.h>
#include <iostream>
#include <memory>
#include "spdlog/spdlog.h"
#pragma once

class DTXUSBTransport
{
private:
    /* data */
public:
    DTXUSBTransport(/* args */);
    ~DTXUSBTransport();

    // """
    // 创建instrument client，用于调用instrument服务的其他接口
    // :param device: 由DeviceService创建的device对象（C对象）
    // :return: instrument client(C对象), 在使用完毕后请务必调用free_client来释放该对象内存
    // """
    instrument_client_t new_client(idevice_t idevice)
    {
        instrument_client_t iclient;
        auto ierr = instrument_client_start_service(idevice, &iclient, "instrument");
        if (ierr != INSTRUMENT_E_SUCCESS)
        {
            return nullptr;
        }
        return iclient;
    }

    // """
    // 释放 instrument client
    // :param client: instrument client(C对象）
    // :return: bool 是否成功
    // """
    bool free_client(instrument_client_t iclient)
    {
        auto ierr = instrument_client_free(iclient);
        return ierr != INSTRUMENT_E_SUCCESS ? false : true;
    }

    // """
    // 向 instrument client 发送整块buffer
    // 成功时表示整块数据都被发出
    // :param client: instrument client(C对象）
    // :param buffer: 数据
    // :return: bool 是否成功
    // """
    bool send_all(instrument_client_t iclient, uint8_t* buf, int buf_len)
    {
        const char* p     = (const char*)buf;
        uint32_t send_len = 0;
        while (send_len < buf_len)
        {
            uint32_t chunk_size = 0;
            auto ierr           = instrument_send_command(iclient, p + send_len, buf_len - send_len, &chunk_size);
            if (ierr != INSTRUMENT_E_SUCCESS)
            {
                spdlog::error("ierr={}, send_all error", ierr);
                return false;
            }
            send_len += chunk_size;
        }
        spdlog::info("send_all succeed, buf_len={}", buf_len);
        return true;
    }
    // """
    // 从 instrument client 接收长度为 length 的 buffer
    // 成功时表示整块数据都被接收
    // :param client: instrument client(C对象）
    // :param length: 数据长度
    // :return: 长度为 length 的 buffer, 失败时返回 None
    // """
    bool recv_all(instrument_client_t iclient, uint8_t* buf, int buf_len, int timeout = -1)
    {
        char* p           = (char*)buf;
        uint32_t recv_len = 0;
        while (recv_len < buf_len)
        {
            int l = buf_len - recv_len > 8192 ? 8192 : buf_len - recv_len;
            char chunk[8192];
            uint32_t chunk_size = 0;
            auto ierr           = instrument_receive(iclient, chunk, l, &chunk_size);
            if (ierr != INSTRUMENT_E_SUCCESS)
            {
                SPDLOG_INFO("ierr={}, recv_all error", ierr);
                return false;
            }

            memcpy(p + recv_len, chunk, chunk_size);
            recv_len += chunk_size;
        }
        return true;
    }
};
