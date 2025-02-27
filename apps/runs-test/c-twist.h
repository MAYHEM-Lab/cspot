#ifndef C_TWIST_H
#define C_TWIST_H

#include <math.h>
#include <stdint.h>

void CTwistInitialize(const uint32_t  seed);
uint32_t CTwistRandomU32();
double CTwistRandom();

#endif

