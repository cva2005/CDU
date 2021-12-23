#pragma message	("@(#)ldc.c")
#include <system.h>
#include "spi/adc/ads1118.h"
#include "tsens/ds1820.h"
#include "pwm/pwm.h"
#include "csu/csu.h"
#include "csu/mtd.h"
#include "lcd.h"

static bool ConMsg;
char LCD[LN][SL];
const uint8_t LADDR[LN] = {LA_0, LA_1, LA_2, LA_3};
ADC_Type ADC_last[4];
Temp_type Temp1_last, Temp2_last;
unsigned char Sec_last=0xFF, cycle_cnt_last, Stg_cnt_last;
unsigned char  Cursor_pos[PR_NUM] = {pr_mode, pr_I, pr_U, pr_time, pr_cycle}, Cursor_point=0;
unsigned char LCD_mode=0;
cap_t Cap; // Capacity of Battery
const char RuCh[64] = {
    0x41,0xA0,0x42,0xA1,0xE0,0x45,0xA3,0xA4,0xA5,0xA6,0x4B,0xA7,0x4D,0x48,0x4F,0xA8,
    0x50,0x43,0x54,0xA9,0xAA,0x58,0xE1,0xAB,0xAC,0xE2,0xAD,0xAE,0x62,0xAF,0xB0,0xB1,
    0x61,0xB2,0xB3,0xB4,0xE3,0x65,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0x6F,0xBE,
    0x70,0x63,0xBF,0x79,0xE4,0x78,0xE5,0xC0,0xC1,0xE6,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7
};

#define TBL_NUM 0xC0
void decd_cpy (char *dst, const char *src, uint8_t n) {
    char ch;
    while (n--) {
        ch = *src++;
        if (ch >= TBL_NUM) *dst = RuCh[ch - TBL_NUM];
        *dst++ = ch;
    }
}

void uint_to_str (uint16_t n, char *p, uint8_t dig) {
    while (dig--) {
        p[dig] = n % 10;
        n /= 10;
    }
}

bool conn_msg (void) {
    return ConMsg;
}

/*char hex_to_ASCII(unsigned char n) {
    if (n > 9) return ('x');
    else return (n + 0x30);
}

void int_to_ASCII(unsigned int val, char *point)
{
point[0]=hex_to_ASCII(val/10000);
point[1]=hex_to_ASCII((val%10000)/1000);
point[2]=hex_to_ASCII((val%1000)/100);
point[3]=hex_to_ASCII((val%100)/10);
point[4]=hex_to_ASCII(val%10);
}*/
// ToDo: check time interval!
static void calc_cap (int8_t sign, uint16_t val, uint16_t k, char *p) {
    uint32_t cur = 0UL;
    if (sign != 0) {
        cur = ((uint32_t)val * (uint32_t)k) / 10000 * 2778UL; //+(uint32_t)b;
        cur = cur / 100000UL;
        Cap.dC += (int)cur * sign;
        if (Cap.dC > 10000) {
            Cap.C++;
            Cap.dC -= 10000;
        } else if (Cap.dC < -10000) {
            Cap.C--;
            Cap.dC += 10000;
        }		
    }
    if (Cap.C < 0) {
        p[0] = '-';
        val = (-1) * Cap.C;	
    } else {
        p[0] = '+';
        val = Cap.C;
    }
    uint_to_str(val, &p[1], 4);
    p[5] = p[4];
    p[4] = ',';
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
    memcpy(point, "Error", 5);
	/*point[0]='E';
	point[1]='r';
	point[2]='r';
	point[3]='o';
	point[4]='r';*/
	}
}

void calc_time (uint8_t h, uint8_t m, uint8_t s, char *p)
{
    if (h < 100 && m < 60 && s < 60) {
        uint_to_str (h, &p[0], 2);
        uint_to_str (m, &p[3], 2);
        uint_to_str (s, &p[6], 2);
        p[2] = p[5] = ':';
    } else {
        memcpy(p, "--:--:--", 8);
    }
}

#if !JTAG_DBGU
void LCD_clear (void) {
    memset(LCD, ' ', sizeof(LCD));
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
	calculate_time(Hour, Min, Sec, &LCD[3][T_P]);
	calc_cap(0, get_adc_res(ADC_MI), Cfg.K_I, &LCD[3][C_P]);
	*mode=0;
	}
    WH2004_string_wr(&LCD[3][0], LA_3, 20);
};

void LCD_wr_set(void) {
    ConMsg = false;
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
void LCD_wr_connect (bool pc) {
    LCD_clear();
    memcpy(&LCD[0][5], "KRON GROUP", 10);
    ConMsg = pc;
    if (pc) decd_cpy(LCD[1], "ПК подключен...", 15);
    else decd_cpy(LCD[1], "Инициализация...", 16);
    decd_cpy(LCD[3], "ЗавN", 4);
    read_num(&LCD[3][4]);
    memcpy(&LCD[3][14], SOFTW_VER, 2);
    for (uint8_t i = 0; i < LN; i++)
        WH2004_string_wr(LCD[i], LADDR[i], SL);
    WH2004_inst_wr(0x0C); // cursor off
}

#define mt Mtd.fld.end // metod time fields
void update_LCD_set (void) {
    uint8_t i;
    if (Cfg.bf1.LCD_ON==0) return;	
    if (LCD_mode!=0) LCD_change_mode(&LCD_mode);
    /* отображение названия метода */
    for (i = 0; i < 13; i++) LCD[0][2 + i] = Mtd.fld.name[i];
    /* отображение номера цикла */
    LCD[0][19]=hex_to_ASCII(Mtd.fld.Cnt);
    LCD[1][19]=hex_to_ASCII(Mtd.fld.NStg);
    /* отображение времени */
    calculate_time(mt.h, mt.m, mt.s, &LCD[3][T_P]);
    /* отображение ёмкости */
    Cap.C = Cap.dC = 0;
    calc_cap(0, get_adc_res(ADC_MI), Cfg.K_I, &LCD[3][C_P]);
    /* отображение тока и напряжения */
    if (SetMode==2) calculate_param(set_Id, Cfg.K_Id, &LCD[1][Is_P]);
    else calculate_param(set_I , Cfg.K_I , &LCD[1][Is_P]);
    calculate_param(set_U, Cfg.K_U, &LCD[2][Us_P]);
    for (i = 0; i < LN; i++) WH2004_string_wr(LCD[i], LADDR[i], SL);
    WH2004_inst_wr(Cursor_pos[Cursor_point]);
    WH2004_inst_wr(0x0F);//Display ON (D=1 diplay on, C=1 cursor on, B=1 blinking on)
    ADC_last[ADC_MU].word=ERR_WCODE;
    ADC_last[ADC_MI].word=ERR_WCODE;
    ADC_last[ADC_DI].word=ERR_WCODE;
    Sec_last=UINT8_MAX;
    cycle_cnt_last=UINT8_MAX;
    Stg_cnt_last=UINT8_MAX;
    Temp1_last.word=ERR_WCODE;
    Temp2_last.word=ERR_WCODE;
}
#endif

void update_LCD_work (void) {
    if (Cfg.bf1.LCD_ON == 0) return;
    WH2004_inst_wr(0x0C);//отключить курсор: Display ON (D=1 diplay on, C=0 cursor off, B=0 blinking off)
    /* ОТОБРАЖЕНИЕ ВЫХОДНОГО ТОКА */
    if (PwmStatus == DISCHARGE) { //если мы в режиме разряда, значит ток надо расчитывать по другому
        if (get_adc_res(ADC_DI) != ADC_last[ADC_MI].word) {
            LCD[1][I_P]='-';
            ADC_last[ADC_MI].word=get_adc_res(ADC_DI);
            calculate_param(get_adc_res(ADC_DI), Cfg.K_Id, &LCD[1][I_P+1]);
            WH2004_string_wr(&LCD[1][I_P],LA_1+I_P, 5); //отобразить
            calculate_param(set_Id , Cfg.K_Id , &LCD[1][Is_P]);
            WH2004_string_wr(&LCD[1][Is_P],LA_1+Is_P, 4); //отобразить
        }		
    } else {
        if (get_adc_res(ADC_MI) !=ADC_last[ADC_MI].word) { //если значение тока изменилось
            LCD[1][I_P]='+';
            ADC_last[ADC_MI].word=get_adc_res(ADC_MI);
            calculate_param(get_adc_res(ADC_MI), Cfg.K_I, &LCD[1][I_P+1]);
            WH2004_string_wr(&LCD[1][I_P],LA_1+I_P, 5); //отобразить
            if (PwmStatus!=STOP) { //Если блок не запущен, то не обнолвять заданные значения, т.к. они зависят от выбранного режима и обновляются в LCD_wr_set
                calculate_param(set_I , Cfg.K_I , &LCD[1][Is_P]);
                WH2004_string_wr(&LCD[1][Is_P],LA_1+Is_P, 4); //отобразить	
            }
        }
    }
    /* ОТОБРАЖЕНИЕ НОМЕРА ЭТАПА */
    if (CsuState != STOP) {
        if (Stg_cnt_last != sCnt) { //на последнем методе проверяется 
            LCD[1][19]=hex_to_ASCII(sCnt+1); //отображать значения на 1 больше, т.к. нумерация этапов начинается с 0
            WH2004_string_wr(&LCD[1][19],LA_1+19, 1); //отобразить
            Stg_cnt_last=sCnt;
        }
    }
    /* ОТОБРАЖЕНИЕ ТЕКУЩЕГО ВРЕМЕНИ */
    if (PwmStatus != STOP) {
        if (Sec_last!=Sec) {		
            if (PwmStatus == DISCHARGE)
                calc_cap(-1, get_adc_res(ADC_DI), Cfg.K_Id, &LCD[3][C_P]);
            else calc_cap(1, get_adc_res(ADC_MI), Cfg.K_I, &LCD[3][C_P]);
            if (LCD_mode == 0) {
                WH2004_string_wr(&LCD[3][C_P],LA_3+C_P, 6);	 
                calculate_time(Hour, Min, Sec, &LCD[3][T_P]);
                WH2004_string_wr(&LCD[3][T_P],LA_3+T_P, 8);
            }
            Sec_last = Sec;
        }
    }	
    /* ОТОБРАЖЕНИЕ ТЕМПЕРАТУРЫ */
    if (LCD_mode != 0) {
        if (Temp1_last.word != Temp1.word) { //если значение изменилось, то отобразить его на дисплее
            calculate_temp(&Temp1.fld.V, &LCD[3][T1_P]); //преобразовать значение в цифры дисплея
            WH2004_string_wr(&LCD[3][T1_P],LA_3+T1_P, 5); //отобразить
            Temp1_last.word=Temp1.word;
        }
        if (Temp2_last.word != Temp2.word) { //если значение изменилось, то отобразить его на дисплее
            calculate_temp(&Temp2.fld.V, &LCD[3][T2_P]); //преобразовать значение в цифры дисплея
            WH2004_string_wr(&LCD[3][T2_P],LA_3+T2_P, 5); //отобразить
            Temp2_last.word=Temp2.word;
        }
    }
    /* ОТОБРАЖЕНИЕ НАПРЯЖЕНИЯ */
    if (get_adc_res(ADC_MU) != ADC_last[ADC_MU].word) { //если значение напряжения изменилось
        calculate_param(get_adc_res(ADC_MU), Cfg.K_U, &LCD[2][U_P]);
        WH2004_string_wr(&LCD[2][U_P],LA_2+U_P, 5); //отобразить
        ADC_last[ADC_MU].word=get_adc_res(ADC_MU);
        calculate_param(set_U , Cfg.K_U , &LCD[2][Us_P]);
        WH2004_string_wr(&LCD[2][Us_P],LA_2+Us_P, 4); //отобразить
    }
    /* ОТОБРАЖЕНИЕ ПОДКЛЮЧЕНИЯ ПК */
#ifndef DEBUG_ALG
    if (rs_active()) {
        if (LCD[2][16] != P_rus) {
            decd_cpy(&LCD[3][16], "ПК", 2);
            goto refr_pc_msg;
        }
    } else {
        if (LCD[2][16]!=' ') {
            memset(&LCD[2][16], ' ', 2);
        refr_pc_msg:
            WH2004_string_wr(&LCD[2][16], LA_2 + 16, 2);
        }
    }
#endif
    /* ОТОБРАЖЕНИЕ НОМЕРА ЦИКЛА */
    if (cycle_cnt_last != cCnt) {
        uint_to_str(cCnt, &LCD[0][S_P], 1);
        WH2004_string_wr(&LCD[0][S_P], LA_0 + S_P, 1);
        cycle_cnt_last=cCnt;
    }
    /* -ОТОБРАЖЕНИЕ ОШИБОК */
    if (Error) {
        memcpy(&LCD[0][2], "Error", 5);
        uint_to_str(Error, &LCD[0][7], 2);
        memset(&LCD[0][9], ' ', 6);
        WH2004_string_wr(&LCD[0][2], LA_0+2, 13);
        memset(LCD[3], ' ', 20);
        switch (Error) {
        case ERR_OVERLOAD:
        case ERR_DISCH_PWR:
            decd_cpy(LCD[3], "Перегрузка", 10);
            if (Error==ERR_OVERLOAD)
                decd_cpy(&LCD[3][11], "заряда", 6);
            else if (Error==ERR_DISCH_PWR)
                decd_cpy(&LCD[3][11], "разряда", 7);
            break;
        case ERR_CONNECTION:
        case ERR_CONNECTION1:
            decd_cpy(LCD[3], "Переполюсовка", 13);
            break;
        case ERR_NO_AKB:
            decd_cpy(LCD[3], "Нет АКБ!!!", 10);
            break;
        case ERR_OVERTEMP1:
            LCD[3][16] = '1';
            goto over_msg;
        case ERR_OVERTEMP2:
            LCD[3][16] = '2';
            goto over_msg;
        case ERR_OVERTEMP3:
            decd_cpy(&LCD[3][16], "внеш", 4);
                LCD[3][16]=v_rus;
                LCD[3][17]=n_rus;
                LCD[3][18]='e';
                LCD[3][19]=sh_rus;
        over_msg:
            decd_cpy(LCD[3], "Перегрев датчик", 15);
            break;
        case ERR_SET:
            decd_cpy(LCD[3], "Задано неверное U", 17);
            break;
        case ERR_Stg:
            decd_cpy(LCD[3], "Этап поврежден", 14);
            break;
        case ERR_OUT:
            decd_cpy(LCD[3], "КЗ выпрямителя", 14);
            break;
        case ERR_ADC:
            decd_cpy(LCD[3], "Ошибка АЦП", 10);
            break;
        case ERR_DM_LOSS:
            decd_cpy(LCD[3], "Обрыв разряд. модуля", 20);
        }
        WH2004_string_wr(&LCD[3][0], LA_3, 20);
    } else {
        if ((LCD[3][0]!='T')&&(LCD[3][0]!='t'))  LCD_wr_set();
        /* ОТОБРАЖЕНИЕ БИТОВ СОСТОЯНИЯ */
#ifndef DEBUG_ALG
        if (PwmStatus==STOP) {
            if (LCD[2][19]!=' ') {
                LCD[2][19]=' ';
                WH2004_string_wr(&LCD[2][19], LA_2+19, 1);
            }
        } else {
            if (pLim) {
                if (LCD[2][19]!='P') {
                    LCD[2][19]='P';
                    WH2004_string_wr(&LCD[2][19], LA_2+19, 1);
                }
        } else {
            if (((I_St)&&(PwmStatus==CHARGE))
                || ((PwmStatus==DISCHARGE) && (get_adc_res(ADC_MU) > (set_U+5)))) {
                if (LCD[2][19]!='I')  {
                    LCD[2][19]='I';	
                    WH2004_string_wr(&LCD[2][19], LA_2+19, 1);
                }			
            } else {
                if (LCD[2][19]!='U') {
                    LCD[2][19]='U';
                    WH2004_string_wr(&LCD[2][19], LA_2+19, 1);
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
    if (CsuState==0) WH2004_inst_wr(0x0F);//Display ON (D=1 diplay on, C=1 cursor on, B=1 blinking on)
}