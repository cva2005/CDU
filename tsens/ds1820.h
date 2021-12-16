#ifdef	__cplusplus
extern "C" {
#endif

#ifndef DS1820_H
#define DS1820_H
#pragma message	("@(#)ds1820.h")

#define READ_ERR_CNT    8
#define Direction_Wr (DDRB|=0x03)
#define Direction_Rd (DDRB&=0xFC)
#define Write_1 PORTB=(PINB|0x03)
#define Write_0 PORTB=(PINB&0xFC)
#define Read_P1  (PINB&0x01)
#define Read_P2  (PINB&0x02)
#define Read_P  (PINB&0x03)  //прочитать значение обоих датчиков

#define Tmask 0x02 //Маска только для датчиков измерющих температуру радиаторов
#define Tmask1 0x01 //Маска только для датчика температуры преобразователя
#define Tmask2 0x02 //Маска только для датчика температуры выпрмяителя

typedef union
	{
	struct
		{
		signed char V;
		unsigned char D;
		}fld;
	unsigned int word;
	}Temp_type;

unsigned char Tmp_Presence_pulse(void);
void Write_Byte_Tmp(unsigned char byte_1wire);
void Read_Byte_Tmp(unsigned char *byte_receive1, unsigned char *byte_receive2);
unsigned char Read_ROM_Tmp(unsigned char *Temperature);
unsigned char CRC8_Tmp(unsigned char data, unsigned char CRC);
unsigned char Read_Current_Temperature(Temp_type *tmp1, Temp_type *tmp2);
unsigned char Thermometr_Convert_Fin(void);
unsigned char Thermometr_Start_Convert(void);
extern unsigned char Err_Thermometr, Err1_cnt, Err2_cnt; //use if need
extern Temp_type Temp1, Temp2;

#ifdef __cplusplus
}
#endif

#endif /* DS1820_H */