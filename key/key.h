#ifndef KEY_H
#define KEY_H
#pragma message	("@(#)key.h")

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    UP_PRESS    = 1,
    DOWN_PRESS  = -1
} up_dw_t;

typedef enum {
    IDX_MODE    = 0,
    IDX_I       = 1,
    IDX_U       = 2,
    IDX_TIME    = 3,
    IDX_CYCLE   = 4,
    PR_NUM      = 5
} curs_idx_t;

#define K5          0x78
#define K4          0xB8
#define K3          0xD8
#define K2          0xE8
#define K1          0xF0
#define ALL_OFF     0xF8
#define KEY_MASK    (PINA & ALL_OFF)

void check_key (void);
void key_power (void);
uint8_t curs_idx (void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_H */