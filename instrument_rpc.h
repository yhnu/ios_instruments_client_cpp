
#include <libimobiledevice/ext.h>
#include <libimobiledevice/libimobiledevice.h>
#include <map>
#include <thread>
#include "DTXClientMixin.h"

#pragma once

typedef void (*selector_callback_t)(void*);
typedef void (*channel_callback_t)(void*);
typedef void (*unhanled_callback_t)(void*);

class instrument_rpc
{
private:
    idevice_t idevice;
    instrument_client_t iclient;

    DTXClientMixin_t idtxclient;
    std::shared_ptr<std::thread> p_recv_thread;
    bool _running;

    std::map<std::string, selector_callback_t> _callbacks;
    int _next_identifier;
    std::map<std::string, int> _channels;
    int _receiver_exiting;
    unhanled_callback_t _unhanled_callback;

    std::map<std::string, selector_callback_t> _channel_callbacks;

public:
    instrument_rpc(/* args */) : idevice(nullptr), idtxclient(nullptr), p_recv_thread(nullptr), _running(false), _next_identifier(1), _receiver_exiting(false), _unhanled_callback(nullptr){};
    ~instrument_rpc();

    bool init(idevice_t device)
    {
        if (device)
        {
            idevice = device;
            if (idtxclient == nullptr)
                idtxclient = std::make_shared<DTXClientMixin>();

            auto iclient = idtxclient->new_client(device);
            if (iclient != nullptr)
            {
                return true;
            }
            else
            {
                SPDLOG_ERROR("idtxclient->new_client is nullptr");
            }
        }
        return false;
    }

    void deinit()
    {
        if (iclient && idtxclient)
        {
            idtxclient->free_client(iclient);
        }
    }

    bool start()
    {
        if (_running == true)
        {
            return true;
        }

        _running      = true;
        p_recv_thread = std::make_shared<std::thread>(_receiver);
        p_recv_thread->join();
        return true;
    }

    bool stop()
    {
        _running = false;
        if (p_recv_thread)
        {
            p_recv_thread->join();
            p_recv_thread = nullptr;
        }
    }

    // """
    // 注册回调, 接受 instrument server 到 client 的远程调用
    // :parma selector: 字符串, selector 名称
    // :param callback: 回调函数, 接受一个参数, 类型是 InstrumentRPCResult 对象实例
    // :return: 无返回值
    // """
    void register_callback(std::string selector, selector_callback_t cb) { _callbacks.insert(std::make_pair(selector, cb)); }

    // """
    // 注册回调, 接受 instrument server 到 client 的远程调用
    // :parma channel: 字符串, channel 名称
    // :param callback: 回调函数, 接受一个参数, 类型是 InstrumentRPCResult 对象实例
    // :return: 无返回值
    // """
    void register_channel_callback(std::string channel, channel_callback_t cb) { _channel_callbacks.insert(std::make_pair(channel, cb)); }

    // """
    // 注册回调, 接受 instrument server 到 client 的远程调用, 处理所以未被处理的消息
    // :param callback: 回调函数, 接受一个参数, 类型是 InstrumentRPCResult 对象实例
    // :return: 无返回值
    // """
    void register_unhandled_callback(unhanled_callback_t cb) { _unhanled_callback = cb; }

    int _make_channel(std::string channel)
    {
        if (channel.empty())
        {
            return 0;
        }

        auto it = _channels.find(channel);
        if (it != _channels.end())
        {
            return it->second;
        }
        int channel_id = _channels.size() + 1;
        // auto dtx       = _call(True, 0, "_requestChannelWithCode:identifier:", channel_id, channel)
    }

private:
    int _call(bool sync, int channel_id, std::string selector, ...) {}

    static void _receiver()
    {
        auto id = std::this_thread::get_id();
        std::cout << id;
    }
};
