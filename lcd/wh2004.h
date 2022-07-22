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

#define INI_N           2
#define LONG_DL         1600
#define SHORT_DL        40

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
#define CURS_BLINK      DISP_CTRL | SET_ON | CUR_ON | BLC_ON
#define CURS_ON         DISP_CTRL | SET_ON | CUR_ON | BLC_OFF
#define CURS_OFF        DISP_CTRL | SET_ON | CUR_OFF | BLC_OFF
#define DISP_OFF        DISP_CTRL | SET_OFF

#define DISP_CLR        0x01

#ifdef JTAG_DBGU
#define Init_wh(...)
#define wh_inst_wr(...)
#define wh_string_wr(...)
#else
void Init_wh (bool enable);
void wh_inst_wr (uint8_t inst);
void wh_string_wr (char *s, uint8_t adr, uint8_t n);
#endif

#define LA_0    0x80
#define LA_1    0xC0
#define LA_2    0x94
#define LA_3    0xD4
#define LN      4 // line num
#define SL      20 // string len

#ifdef __cplusplus
}
#endif

#endif /* WH2004_H */
