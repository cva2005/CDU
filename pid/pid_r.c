#pragma message	("@(#)pid_r.c")
#include <system.h>
#include <string.h>
#include <math.h>
#include "pid_r.h"

void pid_r_init (pid_t *S) {
    memset(S->i, 0, (ST_SIZE + 2) * sizeof(float));
}

float pid_r (pid_t *S, float in) {
    float e[ST_SIZE + 1], D, Df, P, I, out;
    if (fabs(in) < S->Xd) {
        in = 0;
    } else {
        if (in > 0) in -= S->Xd;
        else in += S->Xd;
    }
    e[0] = in;
    memcpy(&e[1], S->i, ST_SIZE * sizeof(float));
#if ST_SIZE == 2
    D = S->Td * (in - 2.0 * e[1] + e[2]);
#elif ST_SIZE == 4
    D = (S->Td / 6.0) * (in + 2.0 * e[1] - 6.0 * e[2] + 2.0 * e[3] + e[4]);
#else
#error "input array size not defined"
#endif
    if (S->Tf) Df = flt_exp(S->d, D, S->Tf);
    else Df = D;
    S->d = Df;
    P = in - e[1];
    if (S->Ti > 0) {
        if (S->Xi > 0) {
            if (fabs(in) > S->Xi) I = 0;
            else goto integer;
        } else {
        integer:
            I = (1.0 / S->Ti) * e[1];
        }
    } else I = 0;
    out = S->u + S->Kp * (P + I + Df);
    S->u = out;
    memcpy(S->i, e, ST_SIZE * sizeof(float));
    return (out);
}