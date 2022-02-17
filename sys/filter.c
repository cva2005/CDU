#pragma message	("@(#)filter.c")

#include <math.h>

float flt_exp (float out, float inp, float tau/*, float bandw*/) {
    //if ((fabs(inp - out) > bandw)) return out;
    return out + (inp - out) / tau;
}