#ifndef LDC_H
#define LDC_H
#pragma message	("@(#)ldc.h")
#include "wh2004.h"

#ifdef	__cplusplus
extern "C" {
#endif

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

void LCD_wr_set(void);
void update_LCD_work(void);
#if JTAG_DBGU
  #define LCD_clear(...)
  #define update_LCD_set(...)
  #define LCD_wr_connect(...)
#else
  void LCD_clear (void);
  void update_LCD_set (void);
  void LCD_wr_connect (bool pc);
#endif
//void update_LCD_error(unsigned char error);


//char hex_to_ASCII(unsigned char N);
void calculate_temp(signed char *temp, char *point);
void calculate_param(unsigned int val, unsigned int k, char *point);
void calculate_time(unsigned char P1, unsigned char P2, unsigned char P3, char *point);
void LCD_change_mode(unsigned char *mode);
bool conn_msg (void);

extern char LCD[LN][SL];
extern unsigned char  Cursor_pos[PR_NUM], Cursor_point;

#ifdef __cplusplus
}
#endif

#endif /* LDC_H */