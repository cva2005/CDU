#pragma message	("@(#)filter.c")

#include <math.h>

#if BAND_WIDTH_USE
float flt_exp (float out, float inp, float tau, float bandw) {
    if (tau == 0 || fabs(inp - out) > bandw) return out;
#else
float flt_exp (float out, float inp, float tau) {
#endif
    if (tau == 0) return out;
    return out + (inp - out) / tau;
}