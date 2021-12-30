#pragma message	("@(#)wh2004.c")
#include <system.h>
#include "wh2004.h"
#include "csu/csu.h"

#if !JTAG_DBGU
void Init_WH2004(unsigned char enable)
{
//DATA_OUT = 0;
//-----------------------------Инициализация-------------------------------
WH2004L_wr_inst;

DATA_OUT=0x30; //Function set (без чтения флага "busy")
WH2004L_enable;
delay_ns;
WH2004L_disable;
delay_us(40);

DATA_OUT=0x30; //Function set (без чтения флага "busy")
WH2004L_enable;
delay_ns;
WH2004L_disable;
delay_us(40);
//----------------------------------запись начальных настроек------------
if (enable)	WH2004_inst_wr(0x38);//Function set (N = 0 1-line displayl; F = 0 5x8 dot character font)

WH2004_inst_wr(0x08);//Display OFF

WH2004_inst_wr(0x01);//Display Clear

if (enable)
	{
	WH2004_inst_wr(0x06);//Entry Mode Set (I/D = 1 Increment by 1; S = 0; No shift)
	//-----------------------------------вкючение ЖКИ--------------------------
	WH2004_inst_wr(0x0F);//Display ON (D=1 diplay on, C=0 cursor off, B=0 blinking off)
	}
}

unsigned char WH2004_wait_ready(void)
{unsigned char BF=0; 
 unsigned int cnt=0;

DATA_OUT=0;    //на выходы все 0
WH2004L_rd_busy;
DDRC=0x00; //все пины D0-D7 настройить на вход;
do
	{
	cnt++;          //увеличить счётчик времени ожидания
	WH2004L_enable; //сигнал включения
	delay_ns; //подождать пока установится данные на шине
	BF=DATA_IN;		//прочитать флаг "busy"
	WH2004L_disable;//убрать сигнал включения
	}
while ( (BF&0x80)&&(cnt<1001) ); //ожидаем готовности WH2004L

//WH2004L_rd_data;
if (cnt>1000) return(0); //Если таймаут вышел, значит WH2004L неисправна*/
return(1);
}

unsigned char WH2004_data_wr(unsigned char data)
{
if (WH2004_wait_ready())
	{
	DATA_OUT=data;
	DDRC=0xFF; //порт настроить как выходы
	WH2004L_wr_data;
	WH2004L_enable;
	delay_ns;
	DATA_OUT=data;
	WH2004L_disable;
	return(1);
	}
return(0);
}

unsigned char WH2004_inst_wr(unsigned char inst)
{
if (WH2004_wait_ready())
	{
	DATA_OUT=inst;
	DDRC=0xFF; //порт настроить как выходы
	WH2004L_wr_inst;
//delay_ns;
	WH2004L_enable;
	delay_ns;
	DATA_OUT=inst;
	WH2004L_disable;
delay_ns;
	return(1);
	}
return(0);
}

void WH2004_string_wr(char *string, unsigned char adr, unsigned char nsym)
{unsigned char cnt=0;

if (WH2004_inst_wr(adr)) //установить курсор в нужную позицию
	{
	for (cnt=0; cnt<nsym; cnt++) WH2004_data_wr(string[cnt]); //записать последовательно все символы
	}	
}
#endif