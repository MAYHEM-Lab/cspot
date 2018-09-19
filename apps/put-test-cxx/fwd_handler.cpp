#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "common.hpp"

#include <iostream>
#include <sstream>
#include <string>

using handler_t = int (&)(WOOF* wf, unsigned long seq, void* data);

extern std::string next_hop;
extern int my_index;

extern "C" int fwd(WOOF*, unsigned long, void* data)
{
    auto elem = *reinterpret_cast<put_elem*>(data);
    elem.stamps[my_index] = get_time();

    if (next_hop == "")
    {
        // done
        for (auto t : elem.stamps)
        {
            std::cout << t << '\n';
        }
        return 0;
    }

    std::ostringstream oss;
    oss << "fwd" << (my_index + 1) << "_fun";

    WooFPut(next_hop.c_str(), oss.str().c_str(), &elem);

    return 0;
}