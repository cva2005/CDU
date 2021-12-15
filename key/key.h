#ifndef KEY_H
#define KEY_H
#pragma message	("@(#)key.h")

#ifdef	__cplusplus
extern "C" {
#endif

unsigned char scan_key(unsigned char *key);
void key_power(void);
void key_set(void);
void key_up(void);
void key_dw(void);

void key_power_LED(void);
void key_U_up(void);
void key_U_dw(void);
void key_I_up(void);
void key_I_dw(void);


#define K5 0x78
#define K4 0xB8
#define K3 0xD8
#define K2 0xE8
#define K1 0xF0
#define KEY_MASK (PINA&0xF8)


extern unsigned char KeyPress;
unsigned char Key_delay, Step;

#ifdef __cplusplus
}
#endif

#endif /* KEY_H */