#ifndef WH2004_H
#define WH2004_H
#pragma message	("@(#)wh2004.h")

#ifdef	__cplusplus
extern "C" {
#endif

#define OUT             PORTA
#define CNTR_PORT       A
#define RS_PIN          0
#define RW_PIN          1
#define EN_PIN          2
#define MODE_PINS       ((1 << RS_PIN) | (1 << RW_PIN))
#define CNTR_PINS       (1 << RS_PIN) | (1 << RW_PIN) | (1 << EN_PIN)
#define LCD_CNTR_PORT   CNTR_PORT
#define LCD_CNTR_PINS   CNTR_PINS

#define WI_MASK         (0 << RS_PIN) | (0 << RW_PIN)
#define RB_MASK         (0 << RS_PIN) | (1 << RW_PIN)
#define WD_MASK         (1 << RS_PIN) | (0 << RW_PIN)
#define RD_MASK         (1 << RS_PIN) | (1 << RW_PIN)
#define WR_INSTR()      OUT = (OUT & ~MODE_PINS) | WI_MASK
#define WR_DATA()       OUT = (OUT & ~MODE_PINS) | WD_MASK
#define RD_BUSY()       OUT = (OUT & ~MODE_PINS) | RB_MASK
#define RD_DATA()       OUT = (OUT & ~MODE_PINS) | RI_MASK

#define EN_ON()         SET_PIN(CNTR_PORT, EN_PIN)
#define EN_OFF()        CLR_PIN(CNTR_PORT, EN_PIN)

#define DATA_PORT       C
#define DATA_OUT        PORTC
#define DATA_IN         PINC
#define BUSY_F          0x80

#define FUNC_SET        0x20
#define BIT_8           0x10
#define BIT_4           0x00
#define LINE_4          0x08
#define LINE_1          0x00
#define DT_5x11         0x04
#define DT_5x8          0x00

#define MODE_SET        0x04
#define INC_MOV         0x02
#define DEC_MOV         0x00
#define SHIFT_EN        0x01
#define NO_SHIFT        0x00

#define DISP_CTRL       0x08
#define SET_ON          0x04
#define SET_OFF         0x00
#define CUR_ON          0x02
#define CUR_OFF         0x00
#define BLC_ON          0x01
#define BLC_OFF         0x00

#define DISP_CLR        0x01

#ifdef JTAG_DBGU
#define Init_WH2004(...)
#define WH2004_inst_wr(...)
#define WH2004_string_wr(...)
#else
void Init_WH2004 (bool enable);
void WH2004_inst_wr (uint8_t inst);
void WH2004_string_wr (char *s, uint8_t adr, uint8_t n);
#endif

#define LA_0    0x80
#define LA_1    0xC0
#define LA_2    0x94
#define LA_3    0xD4
#define LN      4 // line num
#define SL      20 // string len

#define J_rus   163
#define Z_rus   164
#define I_rus   165
#define j_rus   182
#define i_rus   184
#define m_rus   188
#define z_rus   183
#define ya_rus  199
#define d_rus   227
#define D_rus   224
#define u_rus   198
#define l_rus   187
#define _rus    196
#define f_rus   228
#define t_rus   191
#define k_rus   186
#define b_rus   178
#define ch_rus  192
#define n_rus   189
#define p_rus   190
#define ii_rus  195
#define io_rus  181
#define v_rus   179
#define P_rus   168
#define E_rus   175
#define io_rus  181
#define g_rus   180
#define G_rus   161
#define B_rus   160
#define sh_rus  193
#define c_rus   229
#define C_rus   225
 
#ifdef __cplusplus
}
#endif

#endif /* WH2004_H */
