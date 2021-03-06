#pragma message	("@(#)wh.c")
#include <system.h>
#include "wh2004.h"
#include "csu/csu.h"

#ifndef JTAG_DBGU

static void wait_busy (void) {
    uint8_t di; 
    RD_BUSY();
    PORT_AS_INP(DATA_PORT);
    do {
        EN_ON();
        di = DATA_IN;
        EN_OFF();
    } while (di & BUSY_F);
}

static void data_wr (uint8_t data) {
    DATA_OUT = data;
    PORT_AS_OUT(DATA_PORT);
    WR_DATA();
    EN_ON();
    delay_ns();
    EN_OFF();
    wait_busy();
}

void Init_wh (bool enable) {
    WR_INSTR();
    DATA_OUT = FUNC_SET | BIT_8 | LINE_4 | DT_5x8;
    for (uint8_t i = 0; i < 2; i++) {
        EN_ON(); // BF can not be checked before this instruction
        delay_ns();
        EN_OFF();
        delay_us(SHORT_DL);
    }
    uint8_t ctrl;
    if (enable) ctrl = CURS_BLINK;
    else ctrl = DISP_OFF;
    wh_inst_wr(ctrl);
    delay_us(SHORT_DL);
    wh_inst_wr(DISP_CLR);
    delay_us(LONG_DL);
    wh_inst_wr(MODE_SET | INC_MOV | NO_SHIFT);
}

void wh_inst_wr (uint8_t inst) {
    DATA_OUT = inst;
    PORT_AS_OUT(DATA_PORT);
    WR_INSTR();
    EN_ON();
    delay_ns();
    EN_OFF();
    wait_busy();
}

void wh_string_wr (char *s, uint8_t adr, uint8_t n) {
    wh_inst_wr(adr); // set cursor
    for (uint8_t i = 0; i < n; i++)
        data_wr(s[i]); // send char
}
#endif