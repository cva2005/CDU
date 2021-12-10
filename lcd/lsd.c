#pragma message	("@(#)ldc.c")
#include <system.h>
#include "wh2004.h"
#include "lcd.h"
#include "tsens/ds1820.h"
#include "csu/csu.h"
#include "csu/mtd.h"

extern char LCD[4][20];
extern unsigned char Cursor_pos[PR_number], Cursor_point;
extern unsigned char LCD_mode;
extern unsigned char LCD_refresh;

extern unsigned char connect_st;
extern unsigned char CSU_Enable;
extern unsigned char PWM_status, ZR_mode, Error, p_limit;
extern ADC_Type  ADC_ADS1118[4];
extern ADC_Type  ADC_last[4]; //Значения прошлых измерений
extern unsigned int K_I, K_U, K_Id;
extern unsigned char Hour, Min, Sec;
//extern unsigned char Hour_Z, Min_Z, Sec_Z;
extern unsigned int set_I, set_U, set_Id;

extern Temp_type Temp1, Temp2;
extern Temp_type Temp1_last, Temp2_last; //значения температуры на дисплее
extern unsigned char cycle_cnt_last, stage_cnt_last, Sec_last;

extern unsigned char stage_cnt, cycle_cnt;
extern C_type C;

//------------------------------------------------------------------------------------------------
char hex_to_ASCII(unsigned char N)
{
if (N>9) return('x');
else     return(N+0x30);
}

void int_to_ASCII(unsigned int val, char *point)
{
point[0]=hex_to_ASCII(val/10000);
point[1]=hex_to_ASCII((val%10000)/1000);
point[2]=hex_to_ASCII((val%1000)/100);
point[3]=hex_to_ASCII((val%100)/10);
point[4]=hex_to_ASCII(val%10);
}

void calculate_C(int Z, unsigned int val,unsigned int k, char *point)
{uint32_t cur_val=0UL;
 int display_val=0;

if (Z!=0)
	{
	cur_val=((uint32_t)val*(uint32_t)k)/10000*2778UL;//+(uint32_t)b;
	cur_val=cur_val/100000UL;
	C.dC+=((int)cur_val)*Z;
	if (C.dC>10000)
		{
		C.C++;
		C.dC-=10000;
		}		
	if (C.dC<-10000)
		{
		C.C--;
		C.dC+=10000;
		}		
	}
	
if (C.C<0)
	{
	point[0]='-';
	display_val=(-1)*C.C;	
	}
else
	{
	point[0]='+';
	display_val=C.C;
	}
	
point[1]=hex_to_ASCII((display_val%10000)/1000);
point[2]=hex_to_ASCII((display_val%1000)/100);
point[3]=hex_to_ASCII((display_val%100)/10);
point[4]=',';
point[5]=hex_to_ASCII(display_val%10);
}

void calculate_param(unsigned int val, unsigned int k, char *point)
{uint32_t cur_val=0UL;
	
	cur_val=(uint32_t)val*(uint32_t)k;//+(uint32_t)b;
	
	if (((cur_val%1000000UL)/100000UL)>4)
		{
		cur_val+=1000000UL;
		}
	point[0]=hex_to_ASCII(cur_val/100000000UL);//hex_to_ASCII(tmp_data>>4);
	point[1]=hex_to_ASCII((cur_val%100000000UL)/10000000UL);//hex_to_ASCII(tmp_data&0x0F);
	point[2]=',';
	point[3]=hex_to_ASCII((cur_val%10000000UL)/1000000UL);//*/
}

void calculate_temp(signed char *temp, char *point)
{unsigned char deg_val[16]={0,1,1,2,3,3,4,4,5,6,6,7,8,8,9,9};//, tmp_data;
 unsigned char tmp;	
	
if (((unsigned char)temp[0]!=0xFF)||((unsigned char)temp[1]!=0xFF))
	{
	if (temp[0]>=0)
		{
		if (temp[0]>99) point[0]=hex_to_ASCII(temp[0]/100);
		else point[0]='+';
		point[1]=hex_to_ASCII((temp[0]%100)/10);//hex_to_ASCII(tmp_data>>4);
		point[2]=hex_to_ASCII(temp[0]%10);//hex_to_ASCII(tmp_data&0x0F);
		point[3]=',';
		point[4]=hex_to_ASCII(deg_val[(unsigned char)temp[1]]);
		}
	else
		{
		point[0]='-';
		if (((unsigned char)temp[1])!=0) tmp=(-1*temp[0])-1;
		else tmp=-1*temp[0];
		point[1]=hex_to_ASCII(tmp/10);//hex_to_ASCII(tmp_data>>4);
		point[2]=hex_to_ASCII(tmp%10);//hex_to_ASCII(tmp_data&0x0F);
		point[3]=',';
		point[4]=hex_to_ASCII((10-deg_val[(unsigned char)temp[1]])%10);
		}
	}
else
	{
	point[0]='E';
	point[1]='r';
	point[2]='r';
	point[3]='o';
	point[4]='r';
	}
}

void calculate_time(unsigned char P1, unsigned char P2, unsigned char P3, char *point)
{
if ((P1<100)&&(P2<60)&&(P3<60))
	{
	point[0]=hex_to_ASCII(P1/10);
	point[1]=hex_to_ASCII(P1%10);

	point[3]=hex_to_ASCII(P2/10);
	point[4]=hex_to_ASCII(P2%10);	
	
	point[6]=hex_to_ASCII(P3/10);
	point[7]=hex_to_ASCII(P3%10);
	}
else
	{
	point[0]='-';
	point[1]='-';

	point[3]='-';
	point[4]='-';
		
	point[6]='-';
	point[7]='-';
	}

point[2]=':';
point[5]=':';
}

#if !JTAG_DBGU
void LCD_clear(void)
{
unsigned char i, j;	
for (i=0; i<4; i++)
	for (j=0; j<20; j++)
		LCD[i][j]=' ';	
}
#endif
void LCD_change_mode(unsigned char *mode)
{
if (*mode==0)
	{
	LCD[3][0]='t';
	LCD[3][1]='1';
	LCD[3][2]='=';
	LCD[3][8]=';';
	LCD[3][9]=' ';
	LCD[3][10]='t';
	LCD[3][11]='2';
	LCD[3][12]='=';
	LCD[3][18]=' ';
	LCD[3][19]=' ';
	
	Temp1_last.word=0xEFFF;
	Temp2_last.word=0xEFFF;
	*mode=1;
	}	
else
	{
	LCD[3][0]='T';
	LCD[3][1]='=';
	LCD[3][10]=';';
	LCD[3][11]=' ';
	LCD[3][12]='C';
	LCD[3][13]='=';
	calculate_time(Hour, Min, Sec, &LCD[3][T_pos]);
	calculate_C(0, ADC_ADS1118[ADC_MI].word, K_I, &LCD[3][C_pos]);
	*mode=0;
	}
LCD_refresh=0;
WH2004_string_wr(&LCD[3][0], line3, 20);
};

void LCD_wr_set(void)
{
LCD_clear();
LCD[0][0]='P';
LCD[0][1]=':';
//LCD[0][2]='Р';
//LCD[0][3]='а';
//LCD[0][4]='з';
//LCD[0][5]='р';
//LCD[0][6]='я';
//LCD[0][7]='д';
//LCD[0][8]=';';
//LCD[0][9]='U';
//LCD[0][10]='T';
//LCD[0][11]='=';
//LCD[0][12]='0';
//LCD[0][13]='0';
//LCD[0][14]=C_rus;
LCD[0][15]=C_rus;
LCD[0][16]='=';
//LCD[0][17]=':';
LCD[0][18]='/';
//LCD[0][19]='0';

LCD[1][0]='I';
LCD[1][1]='=';
//LCD[1][2]='-';
//LCD[1][3]='-';
//LCD[1][4]=',';
//LCD[1][5]='-';
//LCD[1][6]='A';
LCD[1][7]='A';
LCD[1][8]='(';
//LCD[1][9]='-';
//LCD[1][10]='-';
//LCD[1][11]=',';
//LCD[1][12]='-';
LCD[1][13]='A';
LCD[1][14]=')';
//LCD[1][15]='-';
//LCD[1][16]='|';
LCD[1][17]=E_rus;
LCD[1][18]=':';
//LCD[1][19]='V';


LCD[2][0]='U';
LCD[2][1]='=';
LCD[2][2]='+';
//LCD[2][3]='-';
//LCD[2][4]=',';
//LCD[2][5]='-';
//LCD[2][6]='B';
LCD[2][7]='B';
LCD[2][8]='(';
//LCD[2][9]='-';
//LCD[2][10]='-';
//LCD[2][11]=',';
//LCD[2][12]='-';
LCD[2][13]='B';
LCD[2][14]=')';
//LCD[2][15]='-';
//LCD[2][16]='|';
//LCD[2][17]='m';
//LCD[2][18]='a';
//LCD[2][19]='x';



LCD[3][0]='T';
LCD[3][1]='=';
//LCD[3][2]='=';
//LCD[3][3]=' ';
//LCD[3][4]=':';
//LCD[3][5]=',';
//LCD[3][6]=' ';
//LCD[3][7]=';';
//LCD[3][8]=' ';
//LCD[3][9]='t';
LCD[3][10]=';';
//LCD[3][11]='C';
LCD[3][12]='C';
LCD[3][13]='=';
//LCD[3][14]='r';
//LCD[3][15]=' ';
//LCD[3][16]=' ';
//LCD[3][17]=' ';
//LCD[3][18]=' ';
//LCD[3][19]=' ';
//update_LCD_set();
//WH2004_string_wr(&LCD[0][0], line0, 20);
//WH2004_string_wr(&LCD[1][0], line1, 20);
//WH2004_string_wr(&LCD[2][0], line2, 20);
//WH2004_string_wr(&LCD[3][0], line3, 20);
//WH2004_inst_wr(0x0C);//отключить курсор: Display ON (D=1 diplay on, C=0 cursor off, B=0 blinking off)
//WH2004_inst_wr(0x0F);//Display ON (D=1 diplay on, C=1 cursor on, B=1 blinking on)

update_LCD_set();
}

#if !JTAG_DBGU
void LCD_wr_connect(unsigned char text)
{
LCD_clear();
//LCD[0][0]='I';
//LCD[0][1]='=';
//LCD[0][2]='-';
//LCD[0][3]='-';
//LCD[0][4]=' ';
LCD[0][5]='K';
LCD[0][6]='R';
LCD[0][7]='O';
LCD[0][8]='N';
//LCD[0][9]='"';
LCD[0][10]='G';
LCD[0][11]='R';
LCD[0][12]='O';
LCD[0][13]='U';
LCD[0][14]='P';
//LCD[0][15]='|';
//LCD[0][16]='V';
//LCD[0][17]='V';
//LCD[0][18]='V';
//LCD[0][19]='V';

if (text)
	{
	LCD[1][0]=P_rus;
	LCD[1][1]='K';
	//LCD[1][2]='-';
	LCD[1][3]=p_rus;
	LCD[1][4]='o';
	LCD[1][5]=d_rus;
	LCD[1][6]=k_rus;
	LCD[1][7]=l_rus;
	LCD[1][8]=u_rus;
	LCD[1][9]=ch_rus;
	LCD[1][10]='e';
	LCD[1][11]=n_rus;
	LCD[1][12]='.';
	LCD[1][13]='.';
	LCD[1][14]='.';
	}
else
	{
	LCD[1][0]=I_rus;
	LCD[1][1]=n_rus;
	LCD[1][2]=i_rus;
	LCD[1][3]=c_rus;
	LCD[1][4]=i_rus;
	LCD[1][5]='a';
	LCD[1][6]=l_rus;
	LCD[1][7]=i_rus;
	LCD[1][8]=z_rus;
	LCD[1][9]='a';
	LCD[1][10]=c_rus;
	LCD[1][11]=i_rus;
	LCD[1][12]=ya_rus;
	LCD[1][13]='.';
	LCD[1][14]='.';
	LCD[1][15]='.';
	}

//LCD[2][0]='i';
//LCD[2][1]='=';
//LCD[2][2]='-';
//LCD[2][3]='-';
//LCD[2][4]=',';
//LCD[2][5]='-';
//LCD[2][6]='-';
//LCD[2][7]=Z_rus;
//LCD[2][8]='a';
//LCD[2][9]=v_rus;
//LCD[2][10]='N';
//LCD[2][11]='a';
//LCD[2][12]='1';
//LCD[2][13]='b';
//LCD[2][14]='0';
//LCD[2][15]='0';
//LCD[2][16]='0';
//LCD[2][17]='0';
//LCD[2][18]='0';
//LCD[2][19]='1';



LCD[3][0]=Z_rus;
LCD[3][1]='a';
LCD[3][2]=v_rus;
LCD[3][3]='N';
read_num(&LCD[3][4]);

LCD[3][14]='v';
LCD[3][15]='.';
LCD[3][16]=hex_to_ASCII(Soft_ver);
LCD[3][17]='.';
LCD[3][18]=hex_to_ASCII(Soft_mod/10);
LCD[3][19]=hex_to_ASCII(Soft_mod%10);
//update_LCD_set();
WH2004_string_wr(&LCD[0][0], line0, 20);
WH2004_string_wr(&LCD[1][0], line1, 20);
WH2004_string_wr(&LCD[2][0], line2, 20);
WH2004_string_wr(&LCD[3][0], line3, 20);
WH2004_inst_wr(0x0C);//отключить курсор: Display ON (D=1 diplay on, C=0 cursor off, B=0 blinking off)//*/
}
//========================================================================================================
void update_LCD_set(void)
{unsigned char cnt;
if (CSU_cfg.bit.LCD_ON==0) return;	
	
if (LCD_mode!=0) LCD_change_mode(&LCD_mode);
//-------------------отображение названия метода-----------------------
for (cnt=0; cnt<13; cnt++) LCD[0][2+cnt]=Method.fld.name[cnt];
//-------------------отображение номера цикла--------------
LCD[0][19]=hex_to_ASCII(Method.fld.Cnt);
LCD[1][19]=hex_to_ASCII(Method.fld.Nstage);
//-------------------отображение времени-------------------
calculate_time(Method.fld.Hm, Method.fld.Mm, Method.fld.Sm, &LCD[3][T_pos]);
//------------------отображение ёмкости--------------------
C.C=0; C.dC=0;
calculate_C(0, ADC_ADS1118[ADC_MI].word, K_I, &LCD[3][C_pos]);
//-------------------отображение тока и напряжения----------------
if (ZR_mode==2) calculate_param(set_Id, K_Id, &LCD[1][Is_pos]);
else  			calculate_param(set_I , K_I , &LCD[1][Is_pos]);
calculate_param(set_U, K_U, &LCD[2][Us_pos]);

WH2004_string_wr(&LCD[0][0], line0, 20);
WH2004_string_wr(&LCD[1][0], line1, 20);
WH2004_string_wr(&LCD[2][0], line2, 20);
WH2004_string_wr(&LCD[3][0], line3, 20);
WH2004_inst_wr(Cursor_pos[Cursor_point]);
WH2004_inst_wr(0x0F);//Display ON (D=1 diplay on, C=1 cursor on, B=1 blinking on)

ADC_last[ADC_MU].word=0xFFFF;
ADC_last[ADC_MI].word=0xFFFF;
ADC_last[ADC_DI].word=0xFFFF;
Sec_last=0xFF;
cycle_cnt_last=0xFF;
stage_cnt_last=0xFF;
Temp1_last.word=0xFFFF;
Temp2_last.word=0xFFFF;
}
#endif
//========================================================================================================
void update_LCD_work(void)
{unsigned char i;
if (CSU_cfg.bit.LCD_ON==0) return;

WH2004_inst_wr(0x0C);//отключить курсор: Display ON (D=1 diplay on, C=0 cursor off, B=0 blinking off)
//----------------------------------------------ОТОБРАЖЕНИЕ задаваемых параметров------------------
/*calculate_param(set_U, K_U, &LCD[2][Us_pos]);
WH2004_string_wr(&LCD[2][Us_pos],line2+Us_pos, 5); //отобразить
calculate_param(set_I, K_I, &LCD[1][Is_pos]);
WH2004_string_wr(&LCD[1][Is_pos],line1+Is_pos, 5); //отобразить*/


//----------------------------------------------ОТОБРАЖЕНИЕ ВЫХОДНОГО ТОКА------------------
if (PWM_status==DISCHARGE) //если мы в режиме разряда, значит ток надо расчитывать по другому
	{
	if (ADC_ADS1118[ADC_DI].word!=ADC_last[ADC_MI].word)
		{
		LCD[1][I_pos]='-';
		ADC_last[ADC_MI].word=ADC_ADS1118[ADC_DI].word;
		calculate_param(ADC_ADS1118[ADC_DI].word, K_Id, &LCD[1][I_pos+1]);
		WH2004_string_wr(&LCD[1][I_pos],line1+I_pos, 5); //отобразить

		calculate_param(set_Id , K_Id , &LCD[1][Is_pos]);
		WH2004_string_wr(&LCD[1][Is_pos],line1+Is_pos, 4); //отобразить
		}		
	}
else
	{
	if (ADC_ADS1118[ADC_MI].word!=ADC_last[ADC_MI].word) //если значение тока изменилось
		{
		LCD[1][I_pos]='+';
		ADC_last[ADC_MI].word=ADC_ADS1118[ADC_MI].word;
		calculate_param(ADC_ADS1118[ADC_MI].word, K_I, &LCD[1][I_pos+1]);
		WH2004_string_wr(&LCD[1][I_pos],line1+I_pos, 5); //отобразить
		
		if (PWM_status!=STOP) //Если блок не запущен, то не обнолвять заданные значения, т.к. они зависят от выбранного режима и обновляются в LCD_wr_set
			{
			calculate_param(set_I , K_I , &LCD[1][Is_pos]);
			WH2004_string_wr(&LCD[1][Is_pos],line1+Is_pos, 4); //отобразить	
			}
		}
	}
//----------------------------------------------ОТОБРАЖЕНИЕ ВХОДНОГО ТОКА------------------
/*if (ADC_ADS1118[ADC_PI].word!=ADC_last[ADC_PI].word) //если значение тока первичной цепи изменилось
		{
		ADC_last[ADC_PI].word=ADC_ADS1118[ADC_PI].word;
		calculate_param(ADC_last[ADC_PI].word, K_Ip, &LCD[2][I_pos]);
		WH2004_string_wr(&LCD[2][I_pos],line2+I_pos, 5); //отобразить
		}*/
//----------------------------------------------ОТОБРАЖЕНИЕ НОМЕРА ЭТАПА-----------------		
if (CSU_Enable!=STOP)
	{
	if (stage_cnt_last!=stage_cnt) //на последнем методе проверяется 
		{
		LCD[1][19]=hex_to_ASCII(stage_cnt+1); //отображать значения на 1 больше, т.к. нумерация этапов начинается с 0
		WH2004_string_wr(&LCD[1][19],line1+19, 1); //отобразить
		stage_cnt_last=stage_cnt;
		}
	}
//----------------------------------------------ОТОБРАЖЕНИЕ ТЕКУЩЕГО ВРЕМЕНИ-----------------
if (PWM_status!=STOP)
	{
	if (Sec_last!=Sec) 
		{		
		if (PWM_status==DISCHARGE)
			calculate_C(-1, ADC_ADS1118[ADC_DI].word, K_Id, &LCD[3][C_pos]);
		else
			calculate_C(1, ADC_ADS1118[ADC_MI].word, K_I, &LCD[3][C_pos]);
	
		if (LCD_mode==0)
			{
			WH2004_string_wr(&LCD[3][C_pos],line3+C_pos, 6);	 
			
			calculate_time(Hour, Min, Sec, &LCD[3][T_pos]);
			WH2004_string_wr(&LCD[3][T_pos],line3+T_pos, 8);
			}
		Sec_last=Sec;
		}
	}	
//------------------------------------ОТОБРАЖЕНИЕ ТЕМПЕРАТУРЫ----------------------------------
	if (LCD_mode!=0)
		{
		if (Temp1_last.word!=Temp1.word) //если значение изменилось, то отобразить его на дисплее
			{
			calculate_temp(&Temp1.fld.V, &LCD[3][T1_pos]); //преобразовать значение в цифры дисплея
			WH2004_string_wr(&LCD[3][T1_pos],line3+T1_pos, 5); //отобразить
			Temp1_last.word=Temp1.word;
			}
		if (Temp2_last.word!=Temp2.word) //если значение изменилось, то отобразить его на дисплее
			{
			calculate_temp(&Temp2.fld.V, &LCD[3][T2_pos]); //преобразовать значение в цифры дисплея
			WH2004_string_wr(&LCD[3][T2_pos],line3+T2_pos, 5); //отобразить
			Temp2_last.word=Temp2.word;
			}
		}
//-----------------------------------ОТОБРАЖЕНИЕ НАПРЯЖЕНИЯ-----------------------------------
if (ADC_ADS1118[ADC_MU].word!=ADC_last[ADC_MU].word) //если значение напряжения изменилось
	{	
	calculate_param(ADC_ADS1118[ADC_MU].word, K_U, &LCD[2][U_pos]);
	WH2004_string_wr(&LCD[2][U_pos],line2+U_pos, 5); //отобразить
	ADC_last[ADC_MU].word=ADC_ADS1118[ADC_MU].word;
	
	calculate_param(set_U , K_U , &LCD[2][Us_pos]);
	WH2004_string_wr(&LCD[2][Us_pos],line2+Us_pos, 4); //отобразить
	}
//------------------------------------ОТОБРАЖЕНИЕ ПОДКЛЮЧЕНИЯ ПК---------------------------------
#ifndef DEBUG_ALG
if (connect_st!=0)
	{
	if (LCD[2][16]!=P_rus)
		{
		LCD[2][16]=P_rus;
		LCD[2][17]='K';
		WH2004_string_wr(&LCD[2][16],line2+16, 2); //отобразить
		}
	}
else
	{
	if (LCD[2][16]!=' ')
		{
		LCD[2][16]=' ';
		LCD[2][17]=' ';
		WH2004_string_wr(&LCD[2][16],line2+16, 2); //отобразить
		}
	}
#endif
//------------------------------------ОТОБРАЖЕНИЕ НОМЕРА ЦИКЛА---------------------------------
if (cycle_cnt_last!=cycle_cnt)
	{
	LCD[0][17]=hex_to_ASCII(cycle_cnt);
	WH2004_string_wr(&LCD[0][S_pos],line0+S_pos, 1); //отобразить
	cycle_cnt_last=cycle_cnt;
	}
//------------------------------------ОТОБРАЖЕНИЕ ОШИБОК---------------------------------------	
if (Error)
	{
	LCD[0][2]='E';
	LCD[0][3]='r';
	LCD[0][4]='r';
	LCD[0][5]='o';
	LCD[0][6]='r';
	LCD[0][7]=hex_to_ASCII(Error/10);
	LCD[0][8]=hex_to_ASCII(Error%10);
	for (i=9; i<15; i++) LCD[0][i]=' ';
	WH2004_string_wr(&LCD[0][2], line0+2, 13);
	for (i=0; i<20; i++) LCD[3][i]=' ';
	if ((Error==ERR_OVERLOAD)||(Error==ERR_DISCH_PWR))
		{
		LCD[3][0]=P_rus;
		LCD[3][1]='e';
		LCD[3][2]='p';
		LCD[3][3]='e';
		LCD[3][4]=g_rus;
		LCD[3][5]='p';
		LCD[3][6]='y';
		LCD[3][7]=z_rus;
		LCD[3][8]=k_rus;
		LCD[3][9]='a';
		if (Error==ERR_OVERLOAD)
			{
			LCD[3][11]=z_rus;
			LCD[3][12]='a';
			LCD[3][13]='p';
			LCD[3][14]=ya_rus;
			LCD[3][15]=d_rus;
			LCD[3][16]='a';
			//LCD[3][17]=':';
			//LCD[3][18]='I';
			//LCD[3][19]='U';
			}
		if (Error==ERR_DISCH_PWR)
			{
			LCD[3][11]='p';
			LCD[3][12]='a';
			LCD[3][13]=z_rus;
			LCD[3][14]='p';
			LCD[3][15]=ya_rus;
			LCD[3][16]=d_rus;
			LCD[3][17]='a';
			//LCD[3][18]=':';
			//LCD[3][19]='P';
			}	
		}	
	if ((Error==ERR_CONNECTION)||(Error==ERR_CONNECTION1))
		{
		LCD[3][0]=P_rus;
		LCD[3][1]='e';
		LCD[3][2]='p';
		LCD[3][3]='e';
		LCD[3][4]=p_rus;
		LCD[3][5]='o';
		LCD[3][6]=l_rus;
		LCD[3][7]=u_rus;
		LCD[3][8]='c';
		LCD[3][9]='o';
		LCD[3][10]=v_rus;
		LCD[3][11]=k_rus;
		LCD[3][12]='a';
		}
	if (Error==ERR_NO_AKB)
		{
		LCD[3][0]='H';
		LCD[3][1]='e';
		LCD[3][2]=t_rus;
		LCD[3][3]=' ';
		LCD[3][4]='A';
		LCD[3][5]='K';
		LCD[3][6]=B_rus;
		LCD[3][7]='!';
		LCD[3][8]='!';
		LCD[3][9]='!';
		}
	if ((Error==ERR_OVERTEMP1)||(Error==ERR_OVERTEMP2)||(Error==ERR_OVERTEMP3))
		{
		LCD[3][0]=P_rus;
		LCD[3][1]='e';
		LCD[3][2]='p';
		LCD[3][3]='e';
		LCD[3][4]=g_rus;
		LCD[3][5]='p';
		LCD[3][6]='e';
		LCD[3][7]=v_rus;
		LCD[3][9]=d_rus;
		LCD[3][10]='a';
		LCD[3][11]=t_rus;
		LCD[3][12]=ch_rus;
		LCD[3][13]=i_rus;
		LCD[3][14]=k_rus;
		if (Error==ERR_OVERTEMP1)
			{
			LCD[3][16]='1';
			}
		if (Error==ERR_OVERTEMP2)
			{
			LCD[3][16]='2';
			}
		if (Error==ERR_OVERTEMP3)
			{
			LCD[3][16]=v_rus;
			LCD[3][17]=n_rus;
			LCD[3][18]='e';
			LCD[3][19]=sh_rus;
			}
		}
	if (Error==ERR_SET)
		{
		LCD[3][0]=Z_rus;
		LCD[3][1]='a';
		LCD[3][2]=d_rus;
		LCD[3][3]='a';
		LCD[3][4]=n_rus;
		LCD[3][5]='o';
		LCD[3][7]=n_rus;
		LCD[3][8]='e';
		LCD[3][9]=v_rus;
		LCD[3][10]='e';
		LCD[3][11]='p';
		LCD[3][12]=n_rus;
		LCD[3][13]='o';
		LCD[3][14]='e';
		LCD[3][16]='U';
		}
	if (Error==ERR_STAGE)
		{
		LCD[3][0]=E_rus;
		LCD[3][1]=t_rus;
		LCD[3][2]='a';
		LCD[3][3]=p_rus;
		
		LCD[3][5]=p_rus;
		LCD[3][6]='o';
		LCD[3][7]=v_rus;
		LCD[3][8]='p';
		LCD[3][9]='e';
		LCD[3][10]=j_rus;
		LCD[3][11]=d_rus;
		LCD[3][12]=io_rus;
		LCD[3][13]=n_rus;
		}
	if (Error==ERR_OUT)
		{
		LCD[3][0]='K';
		LCD[3][1]=Z_rus;
		
		LCD[3][3]=v_rus;
		LCD[3][4]=ii_rus;
		LCD[3][5]=p_rus;
		LCD[3][6]='p';
		LCD[3][7]=ya_rus;
		LCD[3][8]=m_rus;
		LCD[3][9]=i_rus;
		LCD[3][10]=t_rus;
		LCD[3][11]='e';
		LCD[3][12]=l_rus;
		LCD[3][13]=ya_rus;
		}
	if (Error==ERR_ADC)
		{
		LCD[3][0]='O';
		LCD[3][1]=sh_rus;
		LCD[3][2]=i_rus;
		LCD[3][3]=b_rus;
		LCD[3][4]=k_rus;
		LCD[3][5]='a';
		
		LCD[3][7]='A';
		LCD[3][8]=C_rus;
		LCD[3][9]=P_rus;
		}
	if (Error==ERR_DM_LOSS)
		{
		LCD[3][0]='O';
		LCD[3][1]=b_rus;
		LCD[3][2]='p';
		LCD[3][3]=ii_rus;
		LCD[3][4]=v_rus;

		LCD[3][6]='p';
		LCD[3][7]='a';
		LCD[3][8]=z_rus;
		LCD[3][9]='p';
		LCD[3][10]=ya_rus;
		LCD[3][11]=d_rus;
		LCD[3][12]='.';
		
		LCD[3][14]=m_rus;
		LCD[3][15]='o';
		LCD[3][16]=d_rus;
		LCD[3][17]='y';
		LCD[3][18]=l_rus;
		LCD[3][19]=ya_rus;
		}		
	WH2004_string_wr(&LCD[3][0], line3, 20);
	}
else
	{
	if ((LCD[3][0]!='T')&&(LCD[3][0]!='t'))
		{
		LCD_wr_set();
		//update_LCD_set();
		}		
//----------------------------------ОТОБРАЖЕНИЕ БИТОВ СОСТОЯНИЯ----------------------------------
#ifndef DEBUG_ALG
	if (PWM_status==STOP)
		{
		if (LCD[2][19]!=' ')
			{
			LCD[2][19]=' ';
			WH2004_string_wr(&LCD[2][19], line2+19, 1);
			}
		}
	else
		{
		if (p_limit)
			{
			if (LCD[2][19]!='P')
				{
				LCD[2][19]='P';
				WH2004_string_wr(&LCD[2][19], line2+19, 1);
				}
			}
		else
			{
			if (((I_St)&&(PWM_status==CHARGE))||
				((PWM_status==DISCHARGE)&&(ADC_ADS1118[ADC_MU].word>(set_U+5))))
				{
				if (LCD[2][19]!='I') 
					{
					LCD[2][19]='I';	
					WH2004_string_wr(&LCD[2][19], line2+19, 1);
					}			
				}
			else
				{
				if (LCD[2][19]!='U')
					{
					LCD[2][19]='U';
					WH2004_string_wr(&LCD[2][19], line2+19, 1);
					}
				}
			}
		}
#endif
	}

//===================================ОТЛАДОЧНАЯ ИНФОРМАЦИЯ==========================================
#ifdef DEBUG_ALG
extern int TEST1, TEST2, TEST3, TEST4;
int_to_ASCII(TEST1, &LCD[0][15]);
WH2004_string_wr(&LCD[0][15], line0+15, 5);
int_to_ASCII(TEST2, &LCD[1][15]);
WH2004_string_wr(&LCD[1][15], line1+15, 5);
int_to_ASCII(TEST3, &LCD[2][15]);
WH2004_string_wr(&LCD[2][15], line2+15, 5);
int_to_ASCII(TEST4, &LCD[3][15]);
WH2004_string_wr(&LCD[3][15], line3+15, 5);
#endif
//==================================================================================================

WH2004_inst_wr(Cursor_pos[Cursor_point]);  //вернуть курсор на место
if (CSU_Enable==0) WH2004_inst_wr(0x0F);//Display ON (D=1 diplay on, C=1 cursor on, B=1 blinking on)
}