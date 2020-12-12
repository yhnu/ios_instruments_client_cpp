#include "dxtmsg.h"

int div_ceil(int p, int q)
{
    return (p + q - 1) / q;
}

int div_floor(int p, int q)
{
    return p / q;
}

int _get_fragment_count_by_length(int length)
{
    if (length <= 65504)  //: # 2**16 - sizeof(DTXMessageHeader)
    {
        return 1;
    }
    return 0;
}
