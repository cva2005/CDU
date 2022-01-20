#ifndef DS1820_H
#define DS1820_H
#pragma message	("@(#)ds1820.h")

#ifdef	__cplusplus
extern "C" {
#endif

#define DDR_18B20       DDRB
#define PORT_18B20      PORTB
#define PIN_18B20       PINB
#define T1_PIN          0x01
#define T2_PIN          0x02
#define T_N             2


#define T_ERROR         0x01
#define T1_ERROR        0x01
#define T2_ERROR        0x02
    
uint8_t get_tmp_res (int16_t *tmp);

#ifdef __cplusplus
}
#endif

#endif /* DS1820_H */