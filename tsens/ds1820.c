#pragma message	("@(#)ds1820.c")
#include <system.h>
#include "ds1820.h"

unsigned char Tmp_Presence_pulse(void)
{//unsigned int i;
 unsigned char data_accept;
 Direction_Wr;
// PDR5_P52=1;
 Write_0;
 //for (i=0; i<300; i++);		//Wait 480 mks (Trsth)
delay_us(480);
 Direction_Rd;
 //for (i=0; i<60; i++);  	//Wait 120mks (Tpdh+Tpdl/2)
delay_us(40);
 data_accept=Read_P;
 //for (i=0; i<300; i++);  	//Wait 520mks ()
delay_us(150);
// PDR5_P52=0;
 //if (data_accept==0) return(1);
 //else return(0);
 return(data_accept);
}

/*
Procedure of write one byte of data into DS18B20 using 1-wire protocol
Parameters: byte of data
*/
void Write_Byte_Tmp(unsigned char byte_1wire)
{unsigned char bit_counter, bit_transmit;
 Write_1;
 Direction_Wr;
 for (bit_counter=0; bit_counter<8; bit_counter++)
 	{bit_transmit=(byte_1wire>>bit_counter)&0x01; //current bit->in last bit
// 	 PDR5_P52=1;
 	 Write_0;			 //Reqwest of write bit
	 //for (i=0; i<5; i++);   //Wait 11 mks (Tlow1)
	 delay_us(11);
//	 DR5_P52=0;

//!!!!!!!!!!!!!!!!----------------------------
//	 Write_0|bit_transmit;//write current bit
	 PORTB=(PINB&0xFC)|(bit_transmit<<1)|bit_transmit;
//----------------------------
	 //for (i=0; i<50; i++);	 //Wait 100mks ()
	 delay_us(100);
//	 PDR5_P52=1;
	 Write_1;			 //End write bit	
 	}//for (bit_counter)
 //for (i=0; i<50; i++);   //Wait when device complede of byte write
 delay_us(40);
 Direction_Rd;
}

/*
Function of read one byte of data from DS18B20 using 1-wire protocol
RETURNS VALUE:
	read byte
*/
void Read_Byte_Tmp(unsigned char *byte_receive1, unsigned char *byte_receive2)
{unsigned char bit_counter,bit_receive1=0,bit_receive2=0;
 *byte_receive1=0;
 *byte_receive2=0;
 Write_1;
 Direction_Wr;
 for (bit_counter=0; bit_counter<8; bit_counter++)
 	{
 //	 PDR5_P52=0;
 	 Write_0;			 //Reqwest of read bit
	 //for (i=0; i<2; i++);   //Wait 3 mks (for device accept reqwest)
	 delay_us(3);
//	 PDR5_P52=1;
	 Direction_Rd; //allow device write data
	 //for (i=0; i<4; i++);   //Wait 8 mks (when device set data bit)
	 delay_us(8);
//	 PDR5_P52=0;
	 bit_receive1=Read_P1; //read 1 bit of data device1
	 bit_receive2=Read_P2>>1; //read 1 bit of data device2
	 *byte_receive1=*byte_receive1|(bit_receive1<<bit_counter); //save receive bit in byte_receive
	 *byte_receive2=*byte_receive2|(bit_receive2<<bit_counter); //save receive bit in byte_receive
	 //for (i=0; i<20; i++); //Wait (when device complited write of data)
	 delay_us(35);
//	 PDR5_P52=1;
	 Write_1;			//End read bit	
	 Direction_Wr;
	 //for (i=0; i<20; i++);   //Wait (pause before next bit will read)
	 delay_us(35);
 	}//for (bit_counter)
// for (i=0; i<150; i++);
 Direction_Rd;
 //*t1=byte_receive1;
 //*t2=byte_receive1;
 return;
}

/*
Function of read temperature from the DS18B20
Parameters: pointer on unsiget char massiv with 8 elements (lenght of code)
RETURNS VALUE:
		fil of massiv
	Status of read:
		00 - data read without errors
		01 - device is not DS18B20
		02 - error of read (maybe second read is need)
*/
//unsigned char Read_ROM_Tmp(unsigned char *Temperature)
//{unsigned char i, CRC=0;
// if (Tmp_Presence_pulse()) return(0x02); //Give present pulse and take answer, if not answer error 2-not device
// Write_Byte_Tmp(0x33);  //wrait command: "Read ROM"
// for (i=0; i<8; i++)
// 	{Temperature[i]=Read_Byte_Tmp(); //Read one byte of data
// 	if (i!=7) CRC=CRC8_Tmp(Temperature[i], CRC);} //calculated CRC for this byte (exclude last byte (8) of receive CRC)
// if (Temperature[0]!=0x28) return(0x03);	  //if first byte!=1 - it's DS18B20
// if (Temperature[7]!=CRC) return(0x01);		  //if calculated CRC!=Read CRC (last byte (8)) - error of read
// return(0x00);							  //if not error 
//}





unsigned char tmp_convert(void)
{unsigned char error;
 error=Tmp_Presence_pulse();//Give present pulse and take answer, if not answer error 1-not device
 
 Write_Byte_Tmp(0xCC); //Send command: "Skip ROM"
 Write_Byte_Tmp(0x44); //Send command: "begin convert"
 return(error);
}

unsigned char Thermometr_Convert_Fin(void)
{unsigned char bit_receive;
 Direction_Wr;
 Write_0;			 //Reqwest of read bit
 //for (i=0; i<2; i++);   //Wait 3 mks (for device accept reqwest)
 delay_us(3);
 Direction_Rd; //allow device write data
 //for (i=0; i<4; i++);   //Wait 8 mks (when device set data bit)
 delay_us(8);
 bit_receive=Read_P; //read status bit in all device
  //for (i=0; i<20; i++); //Wait (when device complited write of data)
 delay_us(35);
 Write_1;			//End read bit	
 Direction_Wr;
 //for (i=0; i<20; i++);   //Wait
 delay_us(35);
 Direction_Rd;
 return (bit_receive); //1 - convert Fined, 0 - convert process
}

uint8_t get_tmp_res (int16_t *tmp) {
    uint8_t i, crc1, crc2, err, b1[9], b2[9];
    crc1 = crc2 = err = 0;
    err = Tmp_Presence_pulse(); //Give present pulse and take answer
    Write_Byte_Tmp(0xCC); //Send command: "Skip ROM"
    Write_Byte_Tmp(0xBE); //Send command: "Read RAM"
    for (i = 0; i < 9; i++) { //Read of 9 data byte
        Read_Byte_Tmp(&b1[i], &b2[i]);
        if (i < 8) {
            crc1 = CRC8_Tmp(b1[i], crc1);
            crc2 = CRC8_Tmp(b2[i], crc2);
        }
    }
    tmp[0] = *(int16_t *)b1;
    tmp[1] = *(int16_t *)b2;
    if (b1[8] != crc1) err |= T1_ERROR;
    if (b2[8] != crc2) err |= T2_ERROR;
    return err;
}

/*
Function of calculated CRC
Parameters: byte of data for calculate, begining CRC
RETURNS VALUE:
		byte of current CRC
*/
unsigned char CRC8_Tmp(unsigned char data, unsigned char CRC)
{
 unsigned char n; unsigned char cnt=8;
 do 
	{
	n=(data^CRC)&0x01;
	CRC>>=1; data>>=1;
	if(n) CRC^=0x8C;
	}
 while(--cnt);
 return(CRC);
}