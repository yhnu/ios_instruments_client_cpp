#include "dxtmsg.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>

int main(int argc, char** argv)
{
    FILE* f = fopen("../0.bin", "r");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        size_t size  = ftell(f);
        uint8_t* bin = new uint8_t[size];

        rewind(f);
        fread(bin, sizeof(uint8_t), size, f);
        fclose(f);

        DTXMessage_t dtx = DTXMessage::from_bytes(bin, size);

        OutBuffer out;
        dtx->to_bytes(out);

        SPDLOG_INFO("{} {}", out.buffer(), out.length());

        FILE* f2 = fopen("../0_out.bin", "wb");
        if (f2)
        {
            fwrite(out.buffer(), out.length(), sizeof(char), f2);
            fclose(f2);
        }
        delete[] bin;
    }
    else
    {
        char err[100];
        perror(err);
        std::cout << "open file error\n" << err;
    }
}