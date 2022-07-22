#pragma message	("@(#)wh2004.c")
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

void Init_WH2004 (bool enable) {
    WR_INSTR();
    DATA_OUT = FUNC_SET | BIT_8 | LINE_4 | DT_5x8;
    for (uint8_t i = 0; i < 2; i++) {
        EN_ON(); // BF can not be checked before this instruction
        delay_ns();
        EN_OFF();
        delay_us(40);
    }
    WH2004_inst_wr(DISP_CTRL | SET_OFF);
    WH2004_inst_wr(DISP_CLR);
    delay_us(1600);
    if (enable) {
        WH2004_inst_wr(MODE_SET | INC_MOV | NO_SHIFT);
        WH2004_inst_wr(DISP_CTRL | SET_ON | CUR_ON | BLC_ON);
    }
}

void WH2004_inst_wr (uint8_t inst) {
    DATA_OUT = inst;
    PORT_AS_OUT(DATA_PORT);
    WR_INSTR();
    EN_ON();
    delay_ns();
    EN_OFF();
    wait_busy();
}

void WH2004_string_wr (char *s, uint8_t adr, uint8_t n) {
    WH2004_inst_wr(adr); // set cursor
    for (uint8_t i = 0; i < n; i++)
        data_wr(s[i]); // send char
}
#endif