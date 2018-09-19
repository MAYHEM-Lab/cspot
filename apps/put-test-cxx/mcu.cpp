//
// Created by fatih on 9/18/18.
//

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
#include <sys/time.h>

extern "C"
{
#include "woofc.h"
#include "woofc-host.h"
#include "woofc-access.h"
}

#include "common.hpp"

const char* args = "f:N:H:W:";
const char *Usage = R"__(put-start -f woof_name
	-H namelog-path to host wide namelog
	-N namespace)__";


void putenv(const char* arg, const std::string& val)
{
    ::putenv(&((arg + ("=" + val))[0]));
}

int main(int argc, char **argv)
{
    std::string ns;
    std::string fname;
    std::string namelog_dir;

    for(int c = getopt(argc,argv,args); c != EOF; c = getopt(argc,argv,args)) {
        switch(c) {
            case 'f':
            case 'W':
                fname = std::string(optarg);
                break;
            case 'N':
                ns = std::string(optarg);
                break;
            case 'H':
                namelog_dir = std::string(optarg);
                break;
            default:
                std::cerr << "unrecognized command " << (char)c << '\n' << Usage << '\n';
                return 1;
        }
    }

    if(fname.empty()) {
        std::cerr << "must specify filename for woof\n" << Usage << '\n';
        return 1;
    }

    if(!namelog_dir.empty()) {
        putenv("WOOF_NAMELOG_DIR", namelog_dir);
    }

    if(!ns.empty()) {
        putenv("WOOFC_DIR", ns);
        ns = "woof://" + ns + "/" + fname;
    } else {
        ns = fname;
    }

    WooFInit();

    put_elem elem{};
    elem.stamps[0] = get_time();

    auto err = WooFPut(ns.c_str(), "fwd1_fun", &elem);
    if(err < 0) {
        std::cerr << "couldn't put woof from " << ns << '\n';
        return 1;
    }
}
