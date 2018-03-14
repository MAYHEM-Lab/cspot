#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

extern "C"
{
#include "woofc.h"
#include "woofc-host.h"
#include "hw.h"
}

#define ARGS "f:N:H:W:"
const char *Usage = "hw -f woof_name\n\
\t-H namelog-path to host wide namelog\n\
\t-N namespace\n";

#include <iostream>
#include <string>

void putenv(const char* arg, const std::string& val)
{
	::putenv(&((arg + ("=" + val))[0]));
}

int main(int argc, char **argv)
{
	std::string ns;
	std::string fname;
	std::string namelog_dir;
	for(int c = getopt(argc,argv,ARGS); c != EOF; c = getopt(argc,argv,ARGS)) {
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
				std::cerr << "unrecognized command " << (char)c << '\n' << Usage << std::flush;
				return 1;
		}
	}

	if(fname.empty()) {
		std::cerr << "must specify filename for woof\n" << Usage << std::flush;
		return 1;
	}

	if(!namelog_dir.empty()) {
		putenv("WOOF_NAMELOG_DIR", namelog_dir);
	}

	if(!ns.empty()) {
		putenv("WOOFC_DIR", ns);
		ns = "woofc://" + ns + "/" + fname;
	} else {
		ns = fname;
	}

	WooFInit();

	auto err = WooFCreate(ns.c_str(), sizeof(HW_EL), 5);
	if(err < 0) {
		std::cerr << "couldn't create woof from " << ns << '\n';
		return 1;
	}

	HW_EL el {};
	strncpy(el.string, "my first bark", sizeof(el.string));

	auto ndx = WooFPut(ns.c_str(), "hw", (void *)&el);

	if(WooFInvalid(err)) {
		std::cerr << "first WooFPut failed for " << ns << '\n';
		return 1;
	}
}

