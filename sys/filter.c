#pragma message	("@(#)filter.c")

float flt_exp (float out, float inp, float tau) {
    return out + (inp - out) / tau;
}