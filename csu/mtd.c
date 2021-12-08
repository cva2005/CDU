#pragma message	("@(#)mtd.c")
#include "csu.h"
#include "mtd.h"
#include "lcd/wh2004.h"
#include "lcd/lcd.h"
#include "net/usart.h"

stg_t Stage;
mtd_t Method;
finish_t finish;
uint8_t pMtd[MTD_N] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t mCnt = 0, sCnt = 0, cCnt = 0;
//unsigned int Method_ARD[MTD_N]={MTD_ADDR,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//unsigned int Wr_ADR = MTD_ADDR;

extern unsigned char finish_cnt;
extern unsigned int pulse_step; //время импульса заряд/разря при импульсном режиме
extern unsigned int dU_time;
extern unsigned char SAVE_Method;
extern unsigned int TEST1, TEST2, TEST3, TEST4;
extern CSU_type CSU_cfg;
extern unsigned char PWM_status, CSU_Enable, ZR_mode, Error;
extern unsigned int set_I, set_Id, set_U, set_UmemC, set_UmemD; //переменный для параметров задаваемого тока и напряжения
extern unsigned int K_I, K_U, K_Id, max_set_I, max_set_Id, max_set_U;
extern unsigned char Hour, Min, Sec, Hour_Stage, Min_Stage, Sec_Stage;
//extern unsigned char Hour_Z, Min_Z, Sec_Z;
extern ADC_Type  ADC_ADS1118[4]; //данные измерений с ADS1118
extern C_type C;
extern unsigned char change_UI;
extern char LCD[4][20];
//extern tx_pack_type tx_pack;
//-------------------------------проверка условий окончания этапа--------------------
unsigned char finish_conditions(void)
{
if (Stage.fld.stop_flag.I) //если есть признак окончания по току
	{
	if (Stage.fld.type==discharge)
		{
		if (ADC_ADS1118[ADC_DI].word<=finish.I) return(1); //при разряде проверить что ток стал меньше минимального допустимого
		}
	else
		{
		if (CSU_Enable==charge)
			{if (ADC_ADS1118[ADC_MI].word<=finish.I) return(1);} //при заряде проверить что ток стал меньше минимально допустимого
		}	
	//если условия выполняются то не сбрасывать счётчик антидребезга
	}
if (Stage.fld.stop_flag.U) //если есть признак окончания по напряженияю
	{
	if ((Stage.fld.type==discharge)||(Stage.fld.type==pause))
		{
		if (ADC_ADS1118[ADC_MU].word<=finish.U) return(1); //при разряде проверить что напряжение меньше допустимого
		}
	else
		{
		if (CSU_Enable==charge)
			{if (ADC_ADS1118[ADC_MU].word>=finish.U) return(1);} //при заряде проверить что напряжение больше допустимого
		}
	//если условия выполняются то не сбрасывать счётчик антидребезга
	}
if (Stage.fld.stop_flag.T)		//если установлен флаг окончания по времени
	{
	if ((Hour_Stage>=Stage.fld.end_H)&&(Min_Stage>=Stage.fld.end_M)&&(Sec_Stage>=Stage.fld.end_S)) //если время этапа вышло
		{
		finish_cnt=0;	//установить флаг окончания метода (сбросить счётчик антидребезка в 0)
		return(1);	
		}
	}
//if (Stage.fld.stop_flag.t)
//if (Stage.fld.stop_flag.C)
if (Stage.fld.stop_flag.dU) //проверить условия падения напряжения
	{
	if (finish.max_U<ADC_ADS1118[ADC_MU].word) //если максимальное зафиксированое напряжение меньше текущего
		{
		finish.max_U=ADC_ADS1118[ADC_MU].word; //запомнить текущее напряжение как максимальное
		dU_time=25000;
		}
	else
		{
		if ((finish.max_U-ADC_ADS1118[ADC_MU].word)>=finish.dU) return(1);  //проверить разница между текущим и максимальным больше дельты?
		}
	if (dU_time==0) return(1);
	}
finish_cnt=30;
return(0);
}

//проверка текущего сосотяния этапа
void stage_status(void)
{
if (Stage.fld.type==pulse) //если задан импульсный режим
	{
	if (pulse_step==0)		//истекло время импульса?
		{
		if (CSU_Enable==discharge) //если был разряд, то сменить его на заряд
			{
			set_U=set_UmemC;		//установить напряжение заряда
			Start_CSU(charge);
			pulse_step=Stage.fld.T_ch*60; //расчитать длительность импульса
			}	
		else
			{
			set_U=set_UmemD;
			Start_CSU(discharge);
			pulse_step=Stage.fld.T_dch*60;
			}
		}	
	}//*/
	
finish_conditions(); //проверить выполняется хоть одно условие окончания этапа (если да то finish_cnt не будет перезагружен)
if (finish_cnt==0) //условия окончания выполняются и вышло время антидребезга?
	{
	if ((stage_cnt+1)<Method.fld.Nstage) //в методе есть информация о следующем этапе?
		{
		stage_cnt++; //переключиться на следующий этап
		read_stage(stage_cnt); //прочитать следующий этап
		calculate_stage();
		change_UI=1;
		if (Stage.fld.type==discharge) 
			{
			if (PWM_status!=discharge) Start_CSU(discharge);
			}
		if ((Stage.fld.type==charge)||(Stage.fld.type==pulse))
			{
			if (PWM_status!=charge) Start_CSU(charge);
			}
		if (Stage.fld.type==pause) 
			{
			if (CSU_Enable!=pause) Start_CSU(pause);//Start_CSU(charge);
			}
		}
	else //если следующего этапа нет
		{
		if ((cycle_cnt<Method.fld.Cnt)||(cycle_cnt>9)) //есть повотрные циклы?
			{
			if (cycle_cnt<10) cycle_cnt++;
			Start_method(0);   //если есть, то запустить метод заново
			}
		else //если нет не выполненых циклов
			{
			Stop_method(); //остановить метод
			}
		}
	}

if (Method.fld.Hm<100)		//Если не бесконечный метод (в методе есть ограниченеи времени)
	{
	if ((Hour>=Method.fld.Hm)&&(Min>=Method.fld.Mm)&&(Sec>=Method.fld.Sm)) //проверить вышло время метода?
		{
		Stop_method(); //если да, то отсановить метод
		}	
	}
}

//расчёт параметров метода
void calculate_method(void)
{uint32_t S=0UL;

 ZR_mode=Stage.fld.type;
 //Hour_Z=Method.fld.Hm;
 //Min_Z=Method.fld.Mm;
 //Sec_Z=Method.fld.Sm;

 S=(((uint32_t)Method.fld.Um)*100000UL)/K_U; //расчитать необходимое напряжение в значениях АЦП
 set_U=(unsigned int)S;

 //if (ZR_mode==discharge)
 S=(((uint32_t)Method.fld.Im)*100000UL)/K_Id; //расчитать ток разряда в значениях АЦП
 set_Id=(unsigned int)S;
 //else
 S=(((uint32_t)Method.fld.Im)*100000UL)/K_I; //расчитать ток заряда в значениях АЦП	
 set_I=(unsigned int)S;
 
 if (Method.fld.Cnt>10) Method.fld.Cnt=10; //Если в методе больше 10 циклов, то ограничить их 10-ю, т.к. 10 - это итак бескончено.
}

//расчёт параметров этапа
void calculate_stage(void)
{uint32_t S=0UL;
//--------------------------------------------

S=(((uint32_t)Method.fld.Im)*((uint32_t)Stage.fld.I_ch)*10UL)/K_I;	
set_I=(unsigned int)S;
if (set_I>max_set_I) set_I=max_set_I;

S=(((uint32_t)Method.fld.Im)*((uint32_t)Stage.fld.I_dch)*10UL)/K_Id;
set_Id=(unsigned int)S;
if (set_Id>max_set_Id) set_Id=max_set_Id;

S=(((uint32_t)Method.fld.Um)*((uint32_t)Stage.fld.U_ch)*10UL)/K_U;
set_UmemC=(unsigned int)S;
if (set_UmemC>max_set_U) set_UmemC=max_set_U;

S=(((uint32_t)Method.fld.Um)*((uint32_t)Stage.fld.U_dch)*10UL)/K_U;
set_UmemD=(unsigned int)S;
if (set_UmemD>max_set_U) set_UmemD=max_set_U;

if (Stage.fld.type==discharge) set_U=set_UmemD;
else set_U=set_UmemC;

if (Stage.fld.stop_flag.U)
	{
	S=(((uint32_t)Method.fld.Um)*((uint32_t)Stage.fld.end_U)*10UL)/K_U;
	finish.U=(unsigned int)S;
	}
if (Stage.fld.stop_flag.dU)
	{
	S=(((uint32_t)Method.fld.Um)*((uint32_t)Stage.fld.end_dU)*10UL)/K_U;
	finish.dU=(unsigned int)S;
	}
if (Stage.fld.stop_flag.I)
	{
	if (Stage.fld.type==discharge)
		S=(((uint32_t)Method.fld.Im)*((uint32_t)Stage.fld.end_I)*10UL)/K_Id;
	else
		S=(((uint32_t)Method.fld.Im)*((uint32_t)Stage.fld.end_I)*10UL)/K_I;
	finish.I=(unsigned int)S;
	}

if (Stage.fld.type==stop_charge) Stage.fld.type=pause;

finish.max_U=0;

pulse_step=Stage.fld.T_ch*60; //установить время импульса заряда в импульсном режиме
Sec_Stage=0;
Min_Stage=0;
Hour_Stage=0;
//C.C=0;
//C.dC=0;
finish_cnt=100; //установить паузу в определении условий окончания
}

void read_method (void) {
    if (read_mtd(pMtd[mCnt], &Method) == false) {
        if (mCnt > MTD_DEF)
            mCnt = 0; //если не причтался не заводской метод, то установить на первый заводской метод
        if (read_mtd(pMtd[mCnt], &Method) == false)
            create_method(mCnt); //если не прочитался заводской метод, то создать его
    }
    sCnt = 0;
    read_stage(0);
    calculate_method();
    cCnt = 0; //установить первый цикл
}

void read_stage (unsigned char stage) {
    if (!EEPROM_read_string(Method_ARD[method_cnt]+SIZE_METHOD+(SIZE_STAGE*stage),
                            SIZE_STAGE, &Stage.byte[0])) Error = ERR_STAGE;
    if (Stage.fld.data_type != 0x55) Error = ERR_STAGE;
}

void Start_method (unsigned char mtd) {
    uint32_t S=0UL, error_calc=0UL;
	Hour=0; Min=0; Sec=0;	
	if (mtd) {
		//рсчитать значения в вольтах
		S=(((uint32_t)set_U)*K_U);//100000UL;
		error_calc=S%1000000UL;
		if ((error_calc/100000UL)>4) S+=1000000UL; //убрать погрешность
		S=(S-error_calc)/100000UL;
		Method.fld.Um=(unsigned int)S;
		//S=(((uint32_t)set_U)*K_U)/100000UL;
		//Method.fld.Um=(unsigned int)S;
		//расчитать ток в амперах
		if (Stage.fld.type==discharge) S=(((uint32_t)set_Id)*K_Id);//100000UL;
		else S=(((uint32_t)set_I)*K_I);//100000UL;
		error_calc=S%1000000UL;
		if ((error_calc/100000UL)>4) S+=1000000UL; //убрать погрешность
		S=(S-error_calc)/100000UL;
		Method.fld.Im=(unsigned int)S;
		//if (Stage.fld.type==discharge)
		//	S=(((uint32_t)set_Id)*K_Id)/100000UL;
		//else
		//	S=(((uint32_t)set_I)*K_I)/100000UL;
		//Method.fld.Im=(unsigned int)S;
		if (SAVE_Method==1) {
			EEPROM_write_string(Method_ARD[method_cnt], SIZE_METHOD, &Method.byte[0]);
			SAVE_Method=0;	
		}			
		cycle_cnt=1;    //установить первый цикл
	}
	stage_cnt=0;	//Установить первый этап
	read_stage(stage_cnt);
	calculate_stage();
	if (Stage.fld.type==discharge) Start_CSU(discharge);
	else {
		if (Stage.fld.type==pause) Start_CSU(pause);	
		else Start_CSU(charge);	
	}	
}

void Stop_method(void) {
    update_LCD_work();
    Stop_CSU(0);
    if (CSU_cfg.bit.LCD_ON) {
        CSU_Enable=1;	
        LCD[0][2]=Z_rus;
        LCD[0][3]='a';
        LCD[0][4]=v_rus;
        LCD[0][5]='e';
        LCD[0][6]='p';
        LCD[0][7]=sh_rus;
        LCD[0][8]='e';
        LCD[0][9]=n_rus;
        LCD[0][10]='o';
        LCD[0][11]=' ';
        LCD[0][12]=' ';
        LCD[0][13]=' ';
        LCD[0][14]=' ';
        WH2004_string_wr(&LCD[0][2],line0+2, 13); //отобразить
	}
}

void create_method (unsigned char method) {
    unsigned char cnt;
    if (method == 0) {
        Method_ARD[0]=METHOD_START_ADR;
        Method.fld.data_type=0xAA;	
        Method.fld.name[0]=Z_rus;
        Method.fld.name[1]='a';
        Method.fld.name[2]='p';
        Method.fld.name[3]=ya_rus;
        Method.fld.name[4]=d_rus;
        for (cnt=5; cnt<18; cnt++) Method.fld.name[cnt]=' ';
        Method.fld.Im=100;
        #ifdef POWER_24
        Method.fld.Im=4000;
        #endif
        #ifdef CHARGER_24
        Method.fld.Im=550;
        #endif
        Method.fld.Um=1440;
        #ifdef POWER_24
        Method.fld.Um=2400;
        #endif
        #ifdef CHARGER_24
        Method.fld.Um=2880;
        #endif
        Method.fld.Hm=10;
        #ifdef POWER_24
        Method.fld.Hm=100;
        #endif
        Method.fld.Mm=0;
        Method.fld.Sm=0;
        Method.fld.Cnt=1;
        Method.fld.Nstage=1;
        EEPROM_write_string(Method_ARD[0], SIZE_METHOD, &Method.byte[0]);
        Stage.fld.data_type=0x55;
        Stage.fld.type=charge;
        Stage.fld.stop_flag.T=0;
        Stage.fld.stop_flag.I=0;
        #ifdef CHARGER_24
        Stage.fld.stop_flag.I=1;
        #endif
        Stage.fld.stop_flag.U=0;
        Stage.fld.stop_flag.dU=0;
        Stage.fld.I_ch=10000;
        Stage.fld.U_ch=10000;
        Stage.fld.I_dch=0;
        Stage.fld.U_dch=0;
        Stage.fld.T_ch=0;
        Stage.fld.T_dch=0;
        Stage.fld.end_I=0;
        #ifdef CHARGER_24
        Stage.fld.end_I=1000;
        #endif
        Stage.fld.end_U=0;
        Stage.fld.end_dU=0;
        Stage.fld.end_Temp=0;
        Stage.fld.end_C=0;
        Stage.fld.end_H=100;
        Stage.fld.end_M=0;
        Stage.fld.end_S=0;
        EEPROM_write_string(Method_ARD[0]+SIZE_METHOD, SIZE_STAGE, &Stage.byte[0]);
	} else if (method==1) {
        //Method_ARD[1]=METHOD_START_ADR+SIZE_METHOD+SIZE_STAGE;
        Method.fld.data_type=0xAA;
        Method.fld.name[0]='P';
        Method.fld.name[1]='a';
        Method.fld.name[2]=z_rus;
        Method.fld.name[3]='p';
        Method.fld.name[4]=ya_rus;
        Method.fld.name[5]=d_rus;
        for (cnt=6; cnt<18; cnt++) Method.fld.name[cnt]=' ';
        Method.fld.Im=100;
        Method.fld.Um=1080;
        Method.fld.Hm=10;
        Method.fld.Mm=0;
        Method.fld.Sm=0;
        Method.fld.Cnt=1;
        Method.fld.Nstage=1;
        EEPROM_write_string(Method_ARD[1], SIZE_METHOD, &Method.byte[0]);
        Stage.fld.data_type=0x55;
        Stage.fld.type=discharge;
        Stage.fld.stop_flag.T=0;
        Stage.fld.stop_flag.I=0;
        Stage.fld.stop_flag.U=0;
        Stage.fld.stop_flag.dU=0;
        Stage.fld.I_ch=0;
        Stage.fld.U_ch=0;
        Stage.fld.I_dch=10000;
        Stage.fld.U_dch=10000;
        Stage.fld.T_ch=0;
        Stage.fld.T_dch=0;
        Stage.fld.end_I=0;
        Stage.fld.end_U=0;
        Stage.fld.end_dU=0;
        Stage.fld.end_Temp=0;
        Stage.fld.end_C=0;
        Stage.fld.end_H=100;
        Stage.fld.end_M=0;
        Stage.fld.end_S=0;
        EEPROM_write_string(Method_ARD[1]+SIZE_METHOD, SIZE_STAGE, &Stage.byte[0]);
	}
}

unsigned int find_free_memory (unsigned char m_cnt) {
    unsigned char result;
    unsigned int ADR;
    method_t MTD;
    ADR=Method_ARD[m_cnt];	
    while (ADR<EEPROM_SIZE) {
        result=EEPROM_read_string(ADR, SIZE_METHOD, &MTD.byte[0]);//прочитать метод из памяти
        if ((!result)||(MTD.fld.data_type!=0xAA)) return(ADR);
        Method_ARD[m_cnt]=ADR;
        m_cnt++;												  //перейти к следующему методу
        ADR=Method_ARD[m_cnt-1]+SIZE_METHOD+(SIZE_STAGE*MTD.fld.Nstage); //посчитать адрес следующего метода
	}
    return (ADR);
}

void delete_all_method (void) {
    unsigned int ADR;
    unsigned char cnt; 
    ADR=METHOD_START_ADR;
    while (ADR<EEPROM_SIZE) {
        EEPROM_write(ADR, 0xFF);
        ADR+=SIZE_METHOD;
	}
    for (cnt=1; cnt<15; cnt++) Method_ARD[cnt]=0;
    Method_ARD[0]=METHOD_START_ADR;
    method_cnt=0; stage_cnt=0; cycle_cnt=0;//номера метода, этапа и цикла
}