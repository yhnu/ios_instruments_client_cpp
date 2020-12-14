/*
 * idevicesyslog.c
 * Relay the syslog of a device to stdout
 *
 * Copyright (c) 2010-2020 Nikias Bassen, All Rights Reserved.
 * Copyright (c) 2009 Martin Szulecki All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define TOOL_NAME "instrument"

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include "dxtmsg.h"

#ifdef WIN32
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#endif

#include <libimobiledevice/ext.h>
#include <libimobiledevice/libimobiledevice.h>
#include "DTXClientMixin.h"
#include "spdlog/spdlog.h"

static int quit_flag          = 0;
static int use_network        = 0;
static int exit_on_disconnect = 1;
static char* udid             = NULL;

static idevice_t device = NULL;

static bool iclient_receive(instrument_client_t iclient, uint8_t* out_buffer, int length, int timeout = -1)
{
    int len = 0;
    while (len < length)
    {
        int l = length - len > 8192 ? 8192 : length - len;
        char receive_buf[8192];
        uint32_t received_len = 0;
        auto ierr             = instrument_receive(iclient, receive_buf, l, &received_len);
        if (ierr != INSTRUMENT_E_SUCCESS)
        {
            std::cout << ierr << " " << __LINE__ << std::endl;
            return false;
        }

        memcpy(out_buffer + len, receive_buf, received_len);
        len += received_len;
        std::cout << "[DEBUG]" << len << "|" << length << "|" << received_len << std::endl;
    }
    return true;
}
#if 0
static int start_logging(void)
{
    idevice_error_t ret = idevice_new_with_options(&device, udid, (use_network) ? IDEVICE_LOOKUP_NETWORK : IDEVICE_LOOKUP_USBMUX);
    if (ret != IDEVICE_E_SUCCESS)
    {
        fprintf(stderr, "Device with udid %s not found!?\n", udid);
        return -1;
    }

    /* cmd channels */
    instrument_client_t iclient;
    instrument_error_t ierr;
    ierr = instrument_client_start_service(device, &iclient, TOOL_NAME);
    if (ierr == INSTRUMENT_E_SUCCESS)
    {
        FILE* f = fopen("../0.bin", "r");
        if (f)
        {
            fseek(f, 0, SEEK_END);
            size_t size = ftell(f);
            char* bin   = new char[size];

            rewind(f);
            fread(bin, sizeof(char), size, f);
            fclose(f);

            uint32_t sents;
            ierr = instrument_send_command(iclient, bin, size, &sents);
            if (ierr != INSTRUMENT_E_SUCCESS)
            {
                std::cout << ierr << std::endl;
            }
            delete[] bin;

            // """
            // 从 instrument client 接收长度为 length 的 buffer
            // 成功时表示整块数据都被接收
            // :param client: instrument client(C对象）
            // :param length: 数据长度
            // :return: 长度为 length 的 buffer, 失败时返回 None
            // """
            int headerSize = sizeof(DTXMessageHeader);
            uint8_t* buf   = (uint8_t*)malloc(headerSize);

            iclient_receive(iclient, buf, headerSize);

            int bodySize = ((DTXMessageHeader*)buf)->length;
            buf          = (uint8_t*)realloc(buf, headerSize + bodySize);

            iclient_receive(iclient, buf + headerSize, bodySize);

            DTXMessage_t dtx = DTXMessage::from_bytes(buf, headerSize + bodySize);

            std::cout << "----------------" << std::endl;
            std::cout << dtx->_buf_size << std::endl;

            FILE* f2 = fopen("../1.bin", "wb");
            if (f2)
            {
                fwrite(dtx->_buf, dtx->_buf_size, sizeof(char), f2);
                fclose(f2);
            }

            instrument_client_free(iclient);

            quit_flag++;
        }
        else
        {
            char err[100];
            perror(err);
            std::cout << "open file error\n" << err;
        }
    }
    else
    {
        std::cout << "[ERROR] " << ierr << std::endl;
    }

    return 0;
}
#else
static int start_logging(void)
{
    idevice_error_t ret = idevice_new_with_options(&device, udid, (use_network) ? IDEVICE_LOOKUP_NETWORK : IDEVICE_LOOKUP_USBMUX);
    if (ret != IDEVICE_E_SUCCESS)
    {
        fprintf(stderr, "Device with udid %s not found!?\n", udid);
        return -1;
    }

    spdlog::info("-----spdlog-----");

    FILE* f = fopen("../0.bin", "r");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        char* bin   = new char[size];

        rewind(f);
        fread(bin, sizeof(char), size, f);
        fclose(f);
        SPDLOG_INFO("{} {}", bin, size);

        DTXClientMixin_t dtx_client = std::make_shared<DTXClientMixin>();
        auto iclient                = dtx_client->new_client(device);
        auto dtx_send               = DTXMessage::from_bytes((uint8_t*)bin, size);
        dtx_client->send_dtx(iclient, *dtx_send);
        delete[] bin;

        // """
        // 从 instrument client 接收长度为 length 的 buffer
        // 成功时表示整块数据都被接收
        // :param client: instrument client(C对象）
        // :param length: 数据长度
        // :return: 长度为 length 的 buffer, 失败时返回 None
        // """
        // int headerSize = sizeof(DTXMessageHeader);
        // uint8_t* buf   = (uint8_t*)malloc(headerSize);

        // iclient_receive(iclient, buf, headerSize);

        // int bodySize = ((DTXMessageHeader*)buf)->length;
        // buf          = (uint8_t*)realloc(buf, headerSize + bodySize);

        // iclient_receive(iclient, buf + headerSize, bodySize);

        // DTXMessage_t dtx = DTXMessage::from_bytes(buf, headerSize + bodySize);
        DTXMessage_t dtx_recv = dtx_client->recv_dtx(iclient);
        if (dtx_recv)
            std::cout << dtx_recv;
        else
            SPDLOG_ERROR("dtx_recv is null");

        FILE* f2 = fopen("../1.bin", "wb");
        if (f2)
        {
            fwrite(dtx_recv->_buf, dtx_recv->_buf_size, sizeof(char), f2);
            fclose(f2);
        }

        instrument_client_free(iclient);

        quit_flag++;
    }
    else
    {
        char err[100];
        perror(err);
        std::cout << "open file error\n" << err;
    }

    return 0;
}
#endif

static void stop_logging(void)
{
    fflush(stdout);

    if (device)
    {
        idevice_free(device);
        device = NULL;
    }
}

static void device_event_cb(const idevice_event_t* event, void* userdata)
{
    printf("[LOG] idevice_event_t.conn_type=%d event=%d udid=%s\n", event->conn_type, event->event, event->udid);
    if (use_network && event->conn_type != CONNECTION_NETWORK)
    {
        // printf(stderr, "[ERROR] use network, but not CONNECTION_NETWORK");
        return;
    }
    else if (!use_network && event->conn_type != CONNECTION_USBMUXD)
    {
        // printf(stderr, "[ERROR] use not network, but CONNECTION_NETWORK");
        return;
    }
    if (event->event == IDEVICE_DEVICE_ADD)
    {
        if (!udid)
        {
            udid = strdup(event->udid);
        }
        if (strcmp(udid, event->udid) == 0)
        {
            if (start_logging() != 0)
            {
                fprintf(stderr, "Could not start logger for udid %s\n", udid);
            }
            printf("[LOG] end start_logging");
        }
    }
    else if (event->event == IDEVICE_DEVICE_REMOVE)
    {
        if ((strcmp(udid, event->udid) == 0))
        {
            stop_logging();
            fprintf(stdout, "[disconnected:%s]\n", udid);
            if (exit_on_disconnect)
            {
                quit_flag++;
            }
        }
    }
}

/**
 * signal handler function for cleaning up properly
 */
static void clean_exit(int sig)
{
    fprintf(stderr, "\nExiting...\n");
    quit_flag++;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, clean_exit);
    signal(SIGTERM, clean_exit);
#ifndef WIN32
    signal(SIGQUIT, clean_exit);
    signal(SIGPIPE, SIG_IGN);
#endif

    // 获取设备列表
    int num                 = 0;
    idevice_info_t* devices = NULL;
    idevice_get_device_list_extended(&devices, &num);
    idevice_device_list_extended_free(devices);
    if (num == 0)
    {
        if (!udid)
        {
            fprintf(stderr, "No device found. Plug in a device or pass UDID with -u to wait for device to be available.\n");
            return -1;
        }
        else
        {
            fprintf(stderr, "Waiting for device with UDID %s to become available...\n", udid);
        }
    }

    idevice_event_subscribe(device_event_cb, NULL);

    while (!quit_flag)
    {
        sleep(1);
    }
    idevice_event_unsubscribe();

    stop_logging();

    return 0;
}
