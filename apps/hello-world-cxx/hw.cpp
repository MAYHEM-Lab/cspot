#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "woofc.h"
#include "hw.hpp"

#include <iostream>
#include <string>

extern "C"
int hw(WOOF *wf, unsigned long seq_no, void *ptr)
{
	HW_EL *el = (HW_EL *)ptr;
	std::cout 
		<< "hello world\nfrom woof " << wf->shared->filename 
		<< " at " << seq_no << " with string: " << el->string << '\n';
	return 1;
}
