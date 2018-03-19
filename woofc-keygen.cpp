#include <czmq.h>
#include <iostream>

int main()
{
	auto cert = zcert_new ();

	std::cout << zcert_public_txt(cert) << '\n';

	zcert_save(cert, "client_cert.txt");

	zcert_destroy(&cert);
}