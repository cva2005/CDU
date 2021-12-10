#pragma message	("@(#)key.c")
#include <sys/config.h>
#include "csu/csu.h"
#include "csu/mtd.h"
#include "lcd/wh2004.h"
#include "lcd/lcd.h"
#include "pwm/pwm.h"
#include "key.h"

//extern CSU_type CSU_cfg;
extern unsigned char  CSU_Enable, ZR_mode;
extern unsigned char self_ctrl; //управление методом заряда производится самостоятельно или удалённо
extern unsigned char Cursor_pos[PR_number], Cursor_point;
extern unsigned int set_I, set_Id, set_U;
extern unsigned char Hour, Min, Sec;
//extern unsigned char Hour_Z, Min_Z;
extern unsigned int K_I, K_U, K_Id, max_set_I, max_set_Id, max_set_U;
extern unsigned char LCD_refresh;
extern unsigned char Key_delay, Step;
extern char LCD[4][20];
extern unsigned char LCD_mode;

extern unsigned char method_cnt;
extern unsigned char SAVE_Method;

extern unsigned int max_pwd_I, max_pwd_U, max_pwd_Id;

unsigned char scan_key(unsigned char *key)
{unsigned char cnt=0, mask;

if (Cfg.bf1.EXT_Id) mask=0x08;
else mask=0;

*key=KEY_MASK|mask; //прочитать кнопки
if (*key!=0xF8)	 //если есть нажатая
	{
	for (cnt=0; cnt<32; cnt++)
		{
		if (*key!=(KEY_MASK|mask)) return (0);	
		}
	return(1);
	}
else return(0);
}

void key_power(void)
{
	if ((CSU_Enable==0)&&(LCD[0][2]!='E')) //если блок не запущен и нет ошибок
		{
		self_ctrl=1;
		LCD_refresh=0;		
		Start_method(1);	
		Key_delay=50;
		}
	else					
		{
		Stop_CSU(0);
		read_method();
		update_LCD_set();
		Key_delay=50;
		}
}

void key_set(void)
{
if (CSU_Enable==0)  //если блок не запущен
	{
	Cursor_point++;
	if (Cursor_point==PR_number) Cursor_point=0;
	}
else 
	{
	Cursor_point=0;
	LCD_change_mode(&LCD_mode);
	}
WH2004_inst_wr(Cursor_pos[Cursor_point]);
Key_delay=20;
}

void key_up(void)
{
	if (CSU_Enable==0)  //если блок не запущен
		{
		switch (Cursor_pos[Cursor_point])
			{
			case pr_mode:
				{
				method_cnt++;
				read_method();
				Key_delay=20;
				break;
				}
			case pr_I:
				{
				if (ZR_mode==DISCHARGE)
					{
					//set_Id++;
					set_Id+=Step;
					if (set_Id>max_set_Id) set_Id=Id_A(0,2);
					}
				else
					{
					//set_I++;
					set_I+=Step;
					if (set_I>max_set_I) set_I=I_A(0,2);
					}
				//Key_delay=2;
				SAVE_Method=1;
				break;
				}
			case pr_U:
				{
				//set_U++;
				set_U+=Step;
				if (set_U>max_set_U) set_U=U_V(0,2);
				//Key_delay=2;
				SAVE_Method=1;
				break;
				}
			case pr_time:
				{
				//Min_Z++;
				if (Method.fld.Hm<100)
					{
					Method.fld.Mm+=Step;
					if (Method.fld.Mm>59)
						{
						Method.fld.Hm++;
						Method.fld.Mm=0;
						}
					}
				Key_delay=10;
				SAVE_Method=1;
				break;
				}
			case pr_cycle:
				{
				Method.fld.Cnt++;
				if (Method.fld.Cnt>10) Method.fld.Cnt=1;
				Key_delay=20;
				SAVE_Method=1;
				}
			}
		update_LCD_set();
		}//if (CSU_Enable==0)
	else
		{
		if (ZR_mode==DISCHARGE)
			{
			set_Id++;
			if (set_Id>max_set_Id) set_Id=Id_A(0,2);
			}
		else
			{
			if (I_St)
				{
				set_I++;
				if (set_I>max_set_I) set_I=I_A(0,2);
				}
			else
				{
				set_U++;
				if (set_U>max_set_U) set_U=U_V(0,2);
				//Key_delay=2;
				}
			}
		delay_ms(3);
		}//else*/
}

void key_dw(void)
{
	if (CSU_Enable==0)  //если блок не запущен
		{
		switch (Cursor_pos[Cursor_point])
			{
			case pr_mode:
				{		
				if (method_cnt>0) method_cnt--;
				read_method();
				Key_delay=20;
				break;
				}
			case pr_I:
				{
				if (ZR_mode==DISCHARGE)
					{
					//set_Id--;
					set_Id-=Step;
					if ((set_Id < Id_A(0,2)) || (set_Id > max_set_Id)) set_Id = max_set_Id;
					}
				else
					{
					//set_I--;
					set_I-=Step;
					if ((set_I < I_A(0,2)) || (set_I > max_set_I)) set_I = max_set_I;
					}
				//	Key_delay=2;
				SAVE_Method=1;
				break;
				}
			case pr_U:
				{
				//set_U--;
				set_U-=Step;
				if ((set_U < U_V(0,2)) || (set_U > max_set_U)) set_U = max_set_U;
				//Key_delay=2;
				SAVE_Method = 1;
				break;
				}
			case pr_time:
				{
				//Min_Z--;
				Method.fld.Mm-=Step;
				if (Method.fld.Mm>59)
					{
					if (Method.fld.Hm>0) Method.fld.Hm--;
					Method.fld.Mm=59;
					}
				SAVE_Method=1;
				Key_delay=10;
				break;
				}
			case pr_cycle:
				{
				Method.fld.Cnt--;
				if (Method.fld.Cnt<1) Method.fld.Cnt=10;	
				SAVE_Method=1;
				Key_delay=20;
				}				
			}
		update_LCD_set();
		}//if (CSU_Enable==0)
	else
		{
		if (ZR_mode==DISCHARGE)
			{
			set_Id--;
			if ((set_Id<Id_A(0,2))||(set_Id>max_set_Id)) set_Id=max_set_Id;
			}
		else
			{
			if (I_St)
				{
				set_I--;
				if ((set_I<I_A(0,2))||(set_I>max_set_I)) set_I=max_set_I;
				}
			else
				{
				set_U--;
				if ((set_U<U_V(0,2))||(set_U>max_set_U)) set_U=max_set_U;
				//Key_delay=2;
				}
			}
		delay_ms(3);
		}//*/
}



void key_power_LED(void)
{
	if (CSU_Enable!=0)  //если блок уже запущен
		{
		Stop_CSU(0);
		}
	else
		{
		Start_CSU(1);
		}
	Key_delay=50;
}

void key_U_up(void)
{
	if (CSU_Enable==1)  //если блок запущен
		{
		if (P_wdU<(max_pwd_U-Step)) P_wdU+=Step;
		else P_wdU=max_pwd_U;
		}//if (CSU_Enable==0)
	Key_delay=1;
}


void key_U_dw(void)
{
	if (CSU_Enable==1)  //если блок запущен
		{
		if (P_wdU>Step) P_wdU-=Step;
		else P_wdU=0;
		}//if (CSU_Enable==0)
	Key_delay=1;
}

void key_I_up(void)
{
	if (CSU_Enable==1)  //если блок запущен
		{
		if (P_wdI<(max_pwd_I-Step)) P_wdI+=Step;
		else P_wdI=max_pwd_I;
		//if (P_wdI<MAX_CK) P_wdI++;
		}//if (CSU_Enable==0)
	Key_delay=1;
}

void key_I_dw(void)
{
	if (CSU_Enable==1)  //если блок запущен
		{
		if (P_wdI>Step) P_wdI-=Step;
		else P_wdI=0;
		//if (P_wdI>0) P_wdI--;
		}//if (CSU_Enable==0)
	Key_delay=1;
}//*/