#ifdef	__cplusplus
extern "C" {
#endif

#ifndef DS1820_H
#define DS1820_H
#pragma message	("@(#)ds1820.h")

#define READ_ERR_CNT    8
#define Direction_Wr    (DDRB|=0x03)
#define Direction_Rd    (DDRB&=0xFC)
#define Write_1         PORTB=(PINB|0x03)
#define Write_0         PORTB=(PINB&0xFC)
#define Read_P1         (PINB&0x01)
#define Read_P2         (PINB&0x02)
#define Read_P          (PINB&0x03)  //прочитать значение обоих датчиков

#define Tmask 0x02 //Маска только для датчиков измерющих температуру радиаторов
#define T1_ERROR    0x01
#define T2_ERROR    0x02
    
uint8_t get_tmp_res (int16_t *tmp);
unsigned char Tmp_Presence_pulse(void);
void Write_Byte_Tmp(unsigned char byte_1wire);
unsigned char Read_ROM_Tmp(unsigned char *Temperature);
unsigned char CRC8_Tmp(unsigned char data, unsigned char CRC);
unsigned char Thermometr_Convert_Fin(void);
unsigned char tmp_convert(void);

#ifdef __cplusplus
}
#endif

#endif /* DS1820_H */