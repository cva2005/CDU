#ifndef WH2004_H
#define WH2004_H
#pragma message	("@(#)wh2004.h")

#ifdef	__cplusplus
extern "C" {
#endif

#define WH2004L_wr_inst PORTA=PORTA&0xFC
#define WH2004L_wr_data PORTA=(PORTA&0xFD)|0x01
#define WH2004L_rd_busy PORTA=(PORTA&0xFE)|0x02
#define WH2004L_rd_data PORTA=PORTA|0x03;

#define WH2004L_enable PORTA=PORTA|0x04
#define WH2004L_disable PORTA=PORTA&0xFB

#define DATA_OUT PORTC
#define DATA_IN PINC

#if JTAG_DBGU
#define Init_WH2004(...)
#define WH2004_wait_ready(...)
#define WH2004_inst_wr(...)
#define WH2004_data_wr(...)
#define WH2004_string_wr(...)
#else
void Init_WH2004 (uint8_t enable);
uint8_t WH2004_wait_ready (void);
uint8_t WH2004_inst_wr (uint8_t inst);
uint8_t WH2004_data_wr (uint8_t data);
void WH2004_string_wr(char *string, uint8_t adr, uint8_t nsym);
#endif

#define LA_0    0x80
#define LA_1    0xC0
#define LA_2    0x94
#define LA_3    0xD4
#define LN      4 // line num
#define SL      20 // string len

#define J_rus  163
#define Z_rus  164
#define I_rus  165
#define j_rus  182
#define i_rus  184
#define m_rus  188
#define z_rus  183
#define ya_rus  199
#define d_rus  227
#define D_rus  224
#define u_rus  198
#define l_rus  187
#define _rus   196
#define f_rus  228
#define t_rus  191
#define k_rus  186
#define b_rus 178
#define ch_rus 192
#define n_rus 189
#define p_rus 190
#define ii_rus 195
#define io_rus 181
#define v_rus 179
#define P_rus 168
#define E_rus 175
#define io_rus 181
#define g_rus 180
#define G_rus 161
#define B_rus 160
#define sh_rus 193
#define c_rus 229
#define C_rus 225
 
#ifdef __cplusplus
}
#endif

#endif /* WH2004_H */
