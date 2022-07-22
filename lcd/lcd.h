#ifndef LDC_H
#define LDC_H
#pragma message	("@(#)ldc.h")
#include "wh2004.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    TIME_MODE   = 0,
    TEMP_MODE   = 1
} lcd_mode_t;

typedef enum {
    IDLE    = 0,
    PLIM    = 1,
    CURR    = 2,
    VOLT    = 3
} lcd_st;

#define SOFTW_VER       "v.9.00"
#define T1_P            3
#define T2_P            13
#define T_DP            (T2_P - T1_P)
#define I_P             2
#define U_P             3
#define Is_P            9
#define Us_P            9
#define T_P             2
#define S_P             17
#define C_P             14
#define PC_CHAR         168

#define PR_MODE         LA_0
#define PR_I            LA_1 
#define PR_U            LA_2
#define PR_TIME         LA_3
#define PR_CYCLE        0x8F

#ifdef JTAG_DBGU
#define decd_cpy(...)
#define lcd_set_curs(...)
#define lcd_wr_set(...)
#define lcd_clear(...)
#define lcd_update_set(...)
#define lcd_wr_connect(...)
#define lcd_stop_msg(...)
#define lcd_start(...)
#define lcd_mode_ch(...)
#define lcd_conn_msg(...) true
#define lcd_update_work(...)
#define lcd_tick_sec(...)
#else
void decd_cpy (char *dst, const char *src, uint8_t n);
void lcd_set_curs (uint8_t idx);
void lcd_wr_set(void);
void lcd_clear (void);
void lcd_update_set (void);
void lcd_wr_connect (bool pc);
void lcd_stop_msg (void);
void lcd_start (void);
void lcd_mode_ch (void);
bool lcd_conn_msg (void);
void lcd_update_work (void);
void lcd_tick_sec (void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* LDC_H */