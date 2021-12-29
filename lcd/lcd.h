#ifndef LDC_H
#define LDC_H
#pragma message	("@(#)ldc.h")
#include "wh2004.h"
#include "tsens/ds1820.h"

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


//#define Mode_pos  6
//#define Dsf_pos  9
#define T1_P        3
#define T2_P        13
#define I_P         2
#define U_P         3
#define Is_P        9
#define Us_P        9
#define T_P         2
#define S_P         17
#define C_P         14
//#define Time_pos  17

#define PR_NUM      5 //число параметров для редактирования

//#define pr_I 0x80
//#define pr_U 0x8A
//#define pr_Set 0x93
#define pr_mode 0x80
#define pr_time 0xD4
//#define pr_z_time 0xC0
//#define pr_r_time 0xCA
#define pr_I 0xC0 
//#define pr_r_I 0x9E
#define pr_U 0x94
//#define pr_r_U 0xDE
#define pr_cycle 0x8F

void lcd_wr_set(void);
void lsd_update_work(void);
#if JTAG_DBGU
    #define lsd_clear(...)
    #define lsd_update_set(...)
    #define lcd_wr_connect(...)
    #define lsd_stop_msg(...)
#else
    void lsd_clear (void);
    void lsd_update_set (void);
    void lcd_wr_connect (bool pc);
    void lsd_stop_msg (void);
#endif

void lsd_mode_ch (void);
bool lsd_conn_msg (void);

extern unsigned char  Cursor_pos[PR_NUM], Cursor_point;
extern int16_t Tmp[TCH];
extern bool CapCalc;

#ifdef __cplusplus
}
#endif

#endif /* LDC_H */