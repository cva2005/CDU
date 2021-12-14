#pragma message	("@(#)mtd.c")
#include <sys/config.h>
#include "lcd/wh2004.h"
#include "lcd/lcd.h"
#include "spi/adc/ads1118.h"
#include "net/net.h"
#include "csu.h"
#include "mtd.h"

stg_t Stg; mtd_t Mtd; fin_t Fin;
uint8_t mCnt = 0, sCnt = 0, cCnt = 0;
uint8_t fCnt;
uint8_t StgNum[MTD_N] = {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint16_t msNum = 0;
bool SaveMtd = false;
unsigned int set_I, set_Id, set_U, set_UmemC, set_UmemD;
unsigned int max_set_U, max_set_I, max_set_Id;
uint8_t Hour = 0, Min = 0, Sec = 0, mSec = 0, Hour_Stg, Min_Stg, Sec_Stg;

extern unsigned int pulse_step; //время импульса заряд/разря при импульсном режиме
extern unsigned int dU_time;
extern unsigned int TEST1, TEST2, TEST3, TEST4;
extern unsigned char PWM_status, CSU_Enable, ZR_mode, Error;
//-------------------------------проверка условий окончания этапа--------------------
unsigned char fin_cond(void)
{
if (Stg.fld.stop_flag.I) //если есть признак окончания по току
	{
	if (Stg.fld.type==DISCHARGE)
		{
		if (ADC_ADS1118[ADC_DI].word<=Fin.I) return(1); //при разряде проверить что ток стал меньше минимального допустимого
		}
	else
		{
		if (CSU_Enable==CHARGE)
			{if (ADC_ADS1118[ADC_MI].word<=Fin.I) return(1);} //при заряде проверить что ток стал меньше минимально допустимого
		}	
	//если условия выполняются то не сбрасывать счётчик антидребезга
	}
if (Stg.fld.stop_flag.U) //если есть признак окончания по напряженияю
	{
	if ((Stg.fld.type==DISCHARGE)||(Stg.fld.type==PAUSE))
		{
		if (ADC_ADS1118[ADC_MU].word<=Fin.U) return(1); //при разряде проверить что напряжение меньше допустимого
		}
	else
		{
		if (CSU_Enable==CHARGE)
			{if (ADC_ADS1118[ADC_MU].word>=Fin.U) return(1);} //при заряде проверить что напряжение больше допустимого
		}
	//если условия выполняются то не сбрасывать счётчик антидребезга
	}
if (Stg.fld.stop_flag.T)		//если установлен флаг окончания по времени
	{
	if ((Hour_Stg>=Stg.fld.end_H)&&(Min_Stg>=Stg.fld.end_M)&&(Sec_Stg>=Stg.fld.end_S)) //если время этапа вышло
		{
		fCnt=0;	//установить флаг окончания метода (сбросить счётчик антидребезка в 0)
		return(1);	
		}
	}
//if (Stg.fld.stop_flag.t)
//if (Stg.fld.stop_flag.C)
if (Stg.fld.stop_flag.dU) //проверить условия падения напряжения
	{
	if (Fin.max_U<ADC_ADS1118[ADC_MU].word) //если максимальное зафиксированое напряжение меньше текущего
		{
		Fin.max_U=ADC_ADS1118[ADC_MU].word; //запомнить текущее напряжение как максимальное
		dU_time=25000;
		}
	else
		{
		if ((Fin.max_U-ADC_ADS1118[ADC_MU].word)>=Fin.dU) return(1);  //проверить разница между текущим и максимальным больше дельты?
		}
	if (dU_time==0) return(1);
	}
fCnt=30;
return(0);
}

//проверка текущего сосотяния этапа
void stg_status(void)
{
if (Stg.fld.type==PULSE) //если задан импульсный режим
	{
	if (pulse_step==0)		//истекло время импульса?
		{
		if (CSU_Enable==DISCHARGE) //если был разряд, то сменить его на заряд
			{
			set_U=set_UmemC;		//установить напряжение заряда
			Start_CSU(CHARGE);
			pulse_step=Stg.fld.T_ch*60; //расчитать длительность импульса
			}	
		else
			{
			set_U=set_UmemD;
			Start_CSU(DISCHARGE);
			pulse_step=Stg.fld.T_dch*60;
			}
		}	
	}//*/
	
fin_cond(); //проверить выполняется хоть одно условие окончания этапа (если да то fCnt не будет перезагружен)
if (fCnt==0) //условия окончания выполняются и вышло время антидребезга?
	{
	if ((sCnt+1) < Mtd.fld.NStg) //в методе есть информация о следующем этапе?
		{
		sCnt++; //переключиться на следующий этап
		read_stg(sCnt); //прочитать следующий этап
		calc_stg();
		change_UI=1;
		if (Stg.fld.type==DISCHARGE) 
			{
			if (PWM_status!=DISCHARGE) Start_CSU(DISCHARGE);
			}
		if ((Stg.fld.type==CHARGE)||(Stg.fld.type==PULSE))
			{
			if (PWM_status!=CHARGE) Start_CSU(CHARGE);
			}
		if (Stg.fld.type==PAUSE) 
			{
			if (CSU_Enable!=PAUSE) Start_CSU(PAUSE);//Start_CSU(charge);
			}
		}
	else //если следующего этапа нет
		{
		if ((cCnt<Mtd.fld.Cnt)||(cCnt>9)) //есть повотрные циклы?
			{
			if (cCnt<10) cCnt++;
			start_mtd(0);   //если есть, то запустить метод заново
			}
		else //если нет не выполненых циклов
			{
			stop_mtd(); //остановить метод
			}
		}
	}

if (Mtd.fld.Hm<100)		//Если не бесконечный метод (в методе есть ограниченеи времени)
	{
	if ((Hour>=Mtd.fld.Hm)&&(Min>=Mtd.fld.Mm)&&(Sec>=Mtd.fld.Sm)) //проверить вышло время метода?
		{
		stop_mtd(); //если да, то отсановить метод
		}	
	}
}

/* расчёт параметров метода */
void calc_mtd (void) {
    ZR_mode=Stg.fld.type;
    set_U = (uint16_t)U_adc(Mtd.fld.Um);
    set_Id = (uint16_t)Id_adc(Mtd.fld.Im);
    set_I = (uint16_t)I_adc(Mtd.fld.Im);
    /* Если в методе больше 10 циклов, то ограничить их 10-ю */
    if (Mtd.fld.Cnt > 10) Mtd.fld.Cnt = 10;
}

/* расчёт параметров этапа */
void calc_stg(void) {
    set_I = (uint16_t)I_m(Mtd.fld.Im, Stg.fld.I_ch);
    if (set_I > max_set_I) set_I = max_set_I;
    set_Id = (uint16_t)Id_m(Mtd.fld.Im, Stg.fld.I_dch);
    if (set_Id > max_set_Id) set_Id = max_set_Id;
    set_UmemC = (uint16_t)U_m(Mtd.fld.Um, Stg.fld.U_ch);
    if (set_UmemC > max_set_U) set_UmemC = max_set_U;
    set_UmemD = (uint16_t)U_m(Mtd.fld.Um, Stg.fld.U_dch);
    if (set_UmemD > max_set_U) set_UmemD = max_set_U;
    if (Stg.fld.type == DISCHARGE) set_U = set_UmemD;
    else set_U = set_UmemC;
    if (Stg.fld.stop_flag.U)
        Fin.U = (uint16_t)U_m(Mtd.fld.Um, Stg.fld.end_U);
    if (Stg.fld.stop_flag.dU)
        Fin.dU = (uint16_t)U_m(Mtd.fld.Um, Stg.fld.end_dU);
    if (Stg.fld.stop_flag.I) {
        if (Stg.fld.type==DISCHARGE)
            Fin.I = (uint16_t)Id_m(Mtd.fld.Im, Stg.fld.end_I);
        else
            Fin.I = (uint16_t)I_m(Mtd.fld.Im, Stg.fld.end_I);
	}
    if (Stg.fld.type==STOP) Stg.fld.type = PAUSE;
    Fin.max_U=0;
    pulse_step = Stg.fld.T_ch * 60; // время импульса заряда в имп. режиме
    Sec_Stg = Min_Stg = Hour_Stg = 0;
    fCnt = 100; //установить паузу в определении условий окончания
}

void read_mtd (void) {
    uint8_t i, n = mCnt;
    for (i = 0; i < mCnt; i++) {
        n += StgNum[i];
    }
    if (eeread_mtd(n, &Mtd) == false) {
        if (mCnt != MTD_DEF) mCnt = n = 0;
        else n = 1;
        //если не причтался не заводской метод, то установить на первый заводской метод
        if (eeread_mtd(n, &Mtd) == false)
            create_mtd(n); //если не прочитался заводской метод, то создать его
    }
    sCnt = 0;
    read_stg(0);
    calc_mtd();
    cCnt = 0; //установить первый цикл
}

void read_stg (unsigned char num) {
    uint8_t i, n = mCnt;
    for (i = 0; i < mCnt; i++) {
        n += StgNum[i];
    }
    n += num;
    if (eeread_stg(n, &Stg) == false) Error = ERR_Stg;
}

void start_mtd (unsigned char num) {
    uint32_t s, error_calc;
	Hour = Min = Sec = 0;	
	if (num) {
		//рсчитать значения в вольтах
		s = set_U * Cfg.K_U;
		error_calc = s % 1000000UL;
		if ((error_calc / 100000UL) > 4) s += 1000000UL; //убрать погрешность
		Mtd.fld.Um = (uint16_t)((s - error_calc) / 100000UL);
		//расчитать ток в амперах
		if (Stg.fld.type == DISCHARGE) s = set_Id * Cfg.K_Id;
		else s = set_I * Cfg.K_I;
		error_calc = s % 1000000UL;
		if ((error_calc / 100000UL) > 4) s += 1000000UL; //убрать погрешность
		Mtd.fld.Im = (uint16_t)((s - error_calc) / 100000UL);
		if (SaveMtd) {
            save_alg(mCnt, &Mtd);
			SaveMtd = false;	
		}			
		cCnt = 1; // set first cycle
	}
	sCnt = 0; // set first stage
	read_stg(sCnt);
	calc_stg();
	if (Stg.fld.type == DISCHARGE) {
        Start_CSU(DISCHARGE);
	} else {
		if (Stg.fld.type==PAUSE) Start_CSU(PAUSE);	
		else Start_CSU(CHARGE);	
	}	
}

void stop_mtd (void) {
    update_LCD_work();
    Stop_CSU(0);
    if (Cfg.bf1.LCD_ON) {
        CSU_Enable = 1;	
        LCD[0][2] = Z_rus;
        LCD[0][3] = 'a';
        LCD[0][4] = v_rus;
        LCD[0][5] = 'e';
        LCD[0][6] = 'p';
        LCD[0][7] = sh_rus;
        LCD[0][8] = 'e';
        LCD[0][9] = n_rus;
        LCD[0][10] = 'o';
        LCD[0][11] = ' ';
        LCD[0][12] = ' ';
        LCD[0][13] = ' ';
        LCD[0][14] = ' ';
        WH2004_string_wr(&LCD[0][2], line0 + 2, 13); // refrash
	}
}

void create_mtd (uint8_t num) {
    if (num < 2) {
        memset(Mtd.fld.name, ' ', sizeof(Mtd.fld.name));
        Mtd.fld.data_type = MTD_ID;
        Mtd.fld.Hm = 10;
        Mtd.fld.Cnt =
        Mtd.fld.NStg = 1;
        Stg.fld.data_type = STG_ID;
        Mtd.fld.Mm =
        Mtd.fld.Sm =
        Stg.fld.stop_flag.T =
        Stg.fld.stop_flag.I =
        Stg.fld.stop_flag.U =
        Stg.fld.stop_flag.dU =
        Stg.fld.T_ch =
        Stg.fld.T_dch =
        Stg.fld.end_I =
        Stg.fld.end_U =
        Stg.fld.end_dU =
        Stg.fld.end_Temp =
        Stg.fld.end_C =
        Stg.fld.end_M =
        Stg.fld.end_S = 0;
        Mtd.fld.Im =
        Stg.fld.end_H = 100;
    } else return;
    if (num == 0) {
        Mtd.fld.name[0] = Z_rus;
        Mtd.fld.name[1] = 'a';
        Mtd.fld.name[2] = 'p';
        Mtd.fld.name[3] = ya_rus;
        Mtd.fld.name[4] = d_rus;
        Mtd.fld.Um = 1440;
        Stg.fld.type = CHARGE;
        Stg.fld.I_ch =
        Stg.fld.U_ch = 10000;
        Stg.fld.I_dch =
        Stg.fld.U_dch = 0;
	} else if (num == 1) {
        Mtd.fld.name[0] = 'P';
        Mtd.fld.name[1] = 'a';
        Mtd.fld.name[2] = z_rus;
        Mtd.fld.name[3] = 'p';
        Mtd.fld.name[4] = ya_rus;
        Mtd.fld.name[5] = d_rus;
        Mtd.fld.Um = 1080;
        Stg.fld.type = DISCHARGE;
        Stg.fld.I_ch =
        Stg.fld.U_ch = 0;
        Stg.fld.I_dch =
        Stg.fld.U_dch = 10000;
	}
    save_alg(num, &Mtd);
    save_alg(num + 1, &Stg);
}

uint8_t find_free (void) {
    uint8_t i, s, n = mCnt;
    for (i = 0; i < mCnt/*MS_N*/; i++) {
        n += StgNum[i];
    }
    mtd_t mtd;
    i = mCnt;
    while (n < MS_N) {
        if (eeread_mtd(n, &mtd) == false) break;
        i++;
        s = mtd.fld.NStg;
        StgNum[i] = s;
        n++;
        n += s;
    }
    return n;
}

void delete_all_mtd (void) {
    eeclr_alg();
    for (uint8_t i = 0; i < MS_N; i++) {
        StgNum[i] = 0;
    }
    mCnt = sCnt = cCnt = 0;
}