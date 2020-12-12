#include "dxtmsg.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>

int main(int argc, char** argv)
{
    FILE* f = fopen("../dtxmsg_1_0.bin", "r");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        size_t size  = ftell(f);
        uint8_t* bin = new uint8_t[size];

        rewind(f);
        fread(bin, sizeof(uint8_t), size, f);
        fclose(f);

        DTXMessage::from_bytes(bin, size);

        delete[] bin;
    }
    else
    {
        char err[100];
        perror(err);
        std::cout << "open file error\n" << err;
    }
}