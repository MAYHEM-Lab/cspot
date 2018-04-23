#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>

extern "C"
{
#include "woofc.h"
#include "woofc-host.h"
	#include "woofc-auth.h"
}

#include "hw.hpp"

const char* args = "f:N:H:i:p:W";
const char *Usage = R"__(hw -f woof_name
	-H namelog-path to host wide namelog
	-N namespace
	-i private key path
	-p woof public key)__";

void putenv(const char* arg, const std::string& val)
{
	::putenv(&((arg + ("=" + val))[0]));
}

int main(int argc, char **argv)
{
	std::string ns;
	std::string fname;
	std::string namelog_dir;
	std::string privkey_path = "/keys/client_cert";
	std::string pubkey = "UkCxC]?G.Pb]5HX61Sig!2c4XyHdy>O55kUiQGpU";

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
			case 'i':
				privkey_path = std::string(optarg);
				break;
			case 'p':
				pubkey = std::string(optarg);
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
		putenv("WOOFC_NAMELOG_DIR", namelog_dir);
	}

	if(!ns.empty()) {
		putenv("WOOFC_DIR", ns);
		ns = "woof://" + ns + "/" + fname;
	} else {
		ns = fname;
	}

	WooFInit();
	WooFAuthInit();
	std::cout << "using key from " << privkey_path << '\n';
	WoofAuthSetPrivateKeyFile(privkey_path.c_str());
	std::cout << "Using public key: " << pubkey << '\n';
	WooFAuthSetPublicKey(ns.c_str(), pubkey.c_str());

	/*auto err = WooFCreate(ns.c_str(), sizeof(HW_EL), 5);
	if(err < 0) {
		std::cerr << "couldn't create woof from " << ns << '\n';
		return 1;
	}*/

	HW_EL el {};
	strncpy(el.string, "my first bark", sizeof(el.string));

	auto ndx = WooFPut(ns.c_str(), "hw_shepherd", (void *)&el);

	if(false){//WooFInvalid(err)) {
		std::cerr << "first WooFPut failed for " << ns << '\n';
		return 1;
	}

	std::cout << "Seqnum: " << ndx << '\n';

	WooFAuthDeinit();
}

