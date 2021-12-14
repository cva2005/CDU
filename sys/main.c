#pragma message	("@(#)main.c")

#include <system.h>
#include "pid/pid_r.h"
#include "net/usart.h"
#include "net/net.h"
#include "pwm/pwm.h"
#include "csu/csu.h"
#include "csu/mtd.h"
#include "tsens/ds1820.h"
#include "lcd/wh2004.h"
#include "lcd/lcd.h"
#include "key/key.h"
#include "spi/adc/ads1118.h"

unsigned long alarm_del;
int TEST1=0, TEST2=0, TEST3=0, TEST4=0;
unsigned char self_ctrl=0; //управление методом заряда производится самостоятельно или удалённо
unsigned int  No_akb_cnt=0, dm_loss_cnt=0;
unsigned int  pid_time = 0;
unsigned int pulse_step; //время импульса заряд/разряд при импульсном режиме
unsigned int dU_time;	//время при котором напряжение неизменно в заряде щелочного АКБ
unsigned int fast_correct=0; //таймаут запрета на быструю коррекцию ШИМ
unsigned char correct_off=0, change_UI=0; //запрет коррекции ШИМ, заданные значения тока и напряжения изменились
unsigned char ERR_Ext=0;		//фильтр на чтения внешнего датчика температуры
unsigned char time_rd=0; //время между чтениями датчиков температуры

#define NO_BATT_TIME    70
#define INF_TAU         50.0f
#define CURR_DT         100.0f
#define ST_TIME         100U
#define DOWN_LIM        100.0f

static void init_gpio (void);

int main (void) {
    init_gpio();
    Init_ExtInt();
    Init_ADS1118();
    ALARM_OUT(0);
    Read_temp(); //Запустить на измерение датчик температуры 
    read_cfg();
    if (Cfg.bf1.LCD_ON) LCD_wr_connect(0);
    if (Cfg.bf1.FAN_CONTROL == 0) {
        FAN(1);
        Read_ADS1118(&ADS1118_chanal); //Прочитать значения АЦП
        delay_ms(500);
        Read_ADS1118(&ADS1118_chanal); //Прочитать значения АЦП
        delay_ms(500);
        read_mtd();
        FAN(0);
    }
    if (Cfg.bf1.LCD_ON) LCD_wr_set();
    init_rs();
    SYS_TMR_ON();
    START_RX(); /* начать прием */
    __enable_interrupt();
    while (1) {	  	
        net_drv(); // проверка драйвера сети
        Err_check(); //Проверить нет ли ошибок
//--------------------------------------------Обработка данных с АЦП	
	Read_ADS1118(&ADS1118_chanal); //Прочитать значения АЦП
	if (ADC_Fin==1) //Если есть оцифрованные каналы
		{
		if ((CSU_cfg.bit.LED_ON==0)||(connect_st!=0))
			Correct_UI(); //Проверить необходимость корректировки тока или/и напряжения
		ADC_Fin=0; //Устанвить флаг ожидания окончания следующей оцифровки
		}
//--------------------------------------------Сохранение данных калибровки		
	if (Clb.id.bit.save == 1) {
		if (!Clb.id.bit.error && (Clb.id.bit.dw_Fin || Clb.id.bit.up_Fin)) {
			save_clb();
			Clb.id.byte = 0;
		}
	}
//--------------------------------------------Обработка данных с Датчика Температуры
	if (time_rd==0) //Если пришло время прочитать датчик температуры
		{
		time_rd=30; //Установить время следующего чтения датчика
		Read_temp(); //Прочитать датчики температуры
		if (CSU_cfg.bit.FAN_CONTROL==0) //Если запрещено внешнее управление вентиляторами
			{
			if ((Temp1.fld.V>FAN_ON_T)||(Temp2.fld.V>FAN_ON_T)||
				((Temp1.fld.V>=-FAN_CND_T)&&(Temp1.fld.V<FAN_CND_T))||
				((Temp2.fld.V>=-FAN_CND_T)&&(Temp2.fld.V<FAN_CND_T))||
				 (Temp1.fld.D==0xFF)||(Temp2.fld.D==0xFF))
				{
				FAN(1);
				}
			else
				{
				if ((Temp1.fld.V<FAN_OFF_T)&&(Temp2.fld.V<FAN_OFF_T)) FAN(0);
				}
			}//*/
		}
	if (CSU_cfg.bit.LCD_ON==1)
		{
		if (connect_st==0) //если подключение прервалось, а значения дисплея не обновлены
			{	
			if (LCD[0][0]!='P')
				{
				if (CSU_cfg.bit.DIAG_WIDE)
					{
					Stop_CSU(0);
					read_Mtd();
					}
				LCD_wr_set();
				}				
			}
		else	 //если подключение появилось, а значения дисплея не обновлены
			{
			if (CSU_cfg.bit.DEBUG_ON==0)
				{
				if (LCD[1][0]!=P_rus) LCD_wr_connect(1);
				}
			}			
		}//if (CSU_cfg.bit.LCD_ON==1)	
			
	if ((connect_st==0)||(CSU_cfg.bit.DEBUG_ON==1))
		{
		if (CSU_cfg.bit.LED_ON)	update_LED();
		if (CSU_cfg.bit.LCD_ON==1)
			{
			if (LCD_refresh==0)
				{
				update_LCD_work();
				if (CSU_Enable==0) LCD_refresh=25;
				}
			}//if (CSU_cfg.bit.LCD_ON==1)

		//if (((PWM_status!=0)||(CSU_Enable==pause))&&(self_ctrl==1)) //Если идёт заряд или разряд и управление зарядом осуществляет ЗРМ самостоятельно
		if ((CSU_Enable!=0)&&(self_ctrl==1)) //Если идёт заряд или разряд и управление зарядом осуществляет ЗРМ самостоятельно
			{
			Stg_status(); //проверить статус этапа: испульсный режим или нет, в случае необходимости изменить исмпульс
			}
			
//---------------------------ОБРАБОТЧИК КНОПОК---------------------				
		if (scan_key(&key_n)!=0) //если есть нажатая кнопка
			{
			if (Key_delay==0)  //если разрешена обработка кнопок
				{
				if (CSU_cfg.bit.LCD_ON)
					{
					if (KeyPress==0) //Если долгое время клавишу удерживали нажатой
						{
						KeyPress=200;
						if (Step==1) Step=10;
						else Step=20;
						}
					if (key_n==K5) key_power();//если нажата кнопка Старт/Стоп
					if (key_n==K4) key_set();//если нажата кнопка Set
					if (key_n==K3) key_up(); //если нажата кнопка вверх
					if (key_n==K2) key_dw(); //если нажата кнопка вниз		
					}
				if (CSU_cfg.bit.LED_ON)
					{
					if (KeyPress==0) //Если долгое время клавишу удерживали нажатой
						{
						KeyPress=100;
						if (Step==1) Step=5;
						else Step=10;
						//if (Step<10) Step++;
						}
					if (key_n==K5) key_power_LED();//если нажата кнопка Старт/Стоп
					if (key_n==K4) key_U_up();//если нажата кнопка Set
					if (key_n==K3) key_U_dw(); //если нажата кнопка вверх
					if (key_n==K2) key_I_up(); //если нажата кнопка вниз	
					if (key_n==K1) key_I_dw(); 
					}
				}//if (Key_delay==0)
			}//if (scan_key(&key_n)!=0)
		else //если нет нажатой кнопки
			{
			KeyPress=100; 
			Step=1;
			}
	//--------------------------------------------АВТОСТАРТ---------------------
		if 	(CSU_cfg2.bit.autostart) //если включён автостарт
			{
			if (PWM_status==STOP) //если преобразователь выключен
				{
				if (autosrart.restart_cnt) //если количество перезапусков неисчерпано
					{
					if (autosrart.restart_time==0) //если истекло время паузы между запусками
						{
						if (autosrart.u_pwm>ADC_ADS1118[ADC_MU].word) //если текущее напряжение на выходе меньше чем напряжение запуска
							{
							if (CSU_Enable==STOP)
								{
								autosrart.restart_time=AUTOSTART_TIME;
								}
							if (Error!=0) 
								{
								if ((autosrart.restart_cnt>0)&&(autosrart.restart_cnt!=0xFF)) autosrart.restart_cnt--;
								//autosrart.restart_cnt=AUTOSTART_CNT;
								}
							key_power();//если нажата кнопка Старт/Стоп
							}
						}
					}
				}
			}
		}//if (connect_st==0)							
		if (ALARM_ON()) { // отключение сигнала окончания работы
			if (++alarm_del == 0x0008ffff) {
				ALARM_OUT(0);
			}
		}
	}//while(1);
    return 0;
}

#if 0 // !PID_CONTROL
void Correct_UI(void)
{unsigned char U_under=0, U_over=0, I_under=0, I_over=0, Id_under=0, Id_over=0;
 unsigned int min_error=0xFFFF, err_U=0, err_I=0, limit_Id=0;

if ((ADS1118_St[ADC_MU]==1)&&((ADS1118_St[ADC_MI]==1)||(ADS1118_St[ADC_DI]==1))) //если оцифрованы одновременно: (U и I) или (U и Id)
	{
		
//------------вычисление ошибки от заданного значения по результатам измерения АЦП----------
	if (ADS1118_St[ADC_MU]==1) //Есть ли даные от внешнего АЦП , то работаем по ним
		{
		if (ADC_ADS1118[ADC_MU].word<=set_U) //текущее напряжение меньше чем надо?
			{
			err_U=set_U-ADC_ADS1118[ADC_MU].word; //считаем ошибку
			if (err_U>4) U_under=1; //max rise_error = (0.1/(maxU/32768))/3 (отличие от заданного значения более, чем на 0,03В)
			}//(ADC_buf[ADC_MU].word<set_U)
		if (ADC_ADS1118[ADC_MU].word>set_U) //текущее напряжение больше чем надо?
			{
			err_U=ADC_ADS1118[ADC_MU].word-set_U;
			if (err_U>4) U_over=1;
			}//if (ADC_buf[ADC_MU].word>set_U)
		if (err_U<min_error) min_error=err_U;
		ADS1118_St[ADC_MU]=0; //Установить признак что данные обработаны
		}
	if (PWM_status==discharge)
		{
		if (ADS1118_St[ADC_DI]==1)
			{
			limit_Id=i_power_limit(P_maxW, set_Id); //Проверка превышения общей мощности
			
			if (ADC_ADS1118[ADC_DI].word<=limit_Id) //текущий ток разряда меньше чем надо?
				{
				err_I=limit_Id-ADC_ADS1118[ADC_DI].word; //считаем ошибку
				if (err_I>4) Id_under=1; //max rise_error = (0.1/(maxId/32768))/3 (отличие от заданного значения более, чем на 0,03А)
				if ((!U_under)&&(err_I<min_error)) min_error=err_I; //если нет просадки напряжения, тогда посмотреть насколько отклонён ток
				}//(ADC_buf[ADC_DI].word<set_dI)
			if (ADC_ADS1118[ADC_DI].word>limit_Id) //текущий ток разряда больше чем надо?
				{
				err_I=ADC_ADS1118[ADC_DI].word-limit_Id;
				if (err_I>4) Id_over=1;
				if (!U_under) //если нет просадки, тогда посмотреть насколько отклонён ток
					{if ((err_I<min_error)||(Id_over)) min_error=err_I;}
				else
					//{if ((err_I>min_error)&&(Id_over)) min_error=err_I;}//если есть превышение и тока и напряжения тогда следить за величиной которая больше.
					{if (err_I>min_error) min_error=err_I;}//если есть превышение и тока и напряжения тогда следить за величиной которая больше.
				}//(ADC_buf[ADC_DI].word>set_dI)
			ADS1118_St[ADC_DI]=0;
			ADS1118_St[ADC_MI]=0;
			}//if (ADS1118_St[ADC_DI]==1)
		} //if (PWM_status==discharge)
	else
		{
		if (ADS1118_St[ADC_MI]==1)
			{
			if (ADC_ADS1118[ADC_MI].word<=set_I) //текущий ток меньше чем надо?
				{
				err_I=set_I-ADC_ADS1118[ADC_MI].word; //считаем ошибку
				if (err_I>8) I_under=1; //max rise_error = (0.1/(maxI/32768))/3 (отличие от заданного значения более, чем на 0,03А)
				//if ((!U_over)&&(err_I<min_error)) min_error=err_I; //если нет превышения напряжения, тогда посмотреть насколько отклонён ток
				}//(ADC_buf[ADC_MU].word<set_U)
			if (ADC_ADS1118[ADC_MI].word>set_I) //текущий ток больше чем надо?
				{
				err_I=ADC_ADS1118[ADC_MI].word-set_I;
				if (err_I>8) I_over=1;
				//if (!U_over) //если нет превышения напряжения, тогда если ток превышает то взять ошибку по току
				//	{if ((err_I<min_error)||(I_over)) min_error=err_I;}
				//else
				//	{if ((err_I>min_error)&&(I_over)) min_error=err_I;} //если есть превышение и тока и напряжения тогда следить за величиной которая больше.
				}//if (ADC_buf[ADC_MU].word>set_U)
			if (err_I<min_error) min_error=err_I;
			ADS1118_St[ADC_MI]=0;
			ADS1118_St[ADC_DI]=0;
			}
		}
//------------------корректирование тока и напряжения при заряде--------------------
	if (correct_off==0) //если нет блокировки на корректировку ШИМ
		{
		PWM_set=1;//Утсановить признак что все параметры стабилизации достигнуты
		if (PWM_status==charge)  //Если ШИМ заряда запущен
			{		
			if (I_St) //если стабилизируется I
				{
				if ((err_I>32)&&(err_U<16)) //ошибка по I значительно больше ошибки по U (если стабилизируется I, но ошибка по I значительно больше ошибки по U)
					{
					U_over=1; //Заставить снижаться напряжение, чтобы перейти в режим стабилизации напряжения
					if ((change_UI)&&(CSU_cfg.bit.GroupM==0)) Correct_PWM(0x23); //если была команда на имзенения праметров стабилизации, то пересчитать заново оба PWM (перевести в режим стабилизации U)
					}
				if (I_under)
					{
					if (P_wdI<MAX_CK) P_wdI++;
					PWM_set=0;
					}
				}
			else //если стабилизируется U
				{
				if ((err_U>32)&&(err_I<16))  //ошибка по U значительно больше ошибки по I (если стабилизируется U, но ошибка по U значительно больше ошибки по I)
					{
					if ((change_UI)&&(CSU_cfg.bit.GroupM==0)) Correct_PWM(0x13); //если была команда на имзенения праметров стабилизации, то пересчитать заново оба PWM (перевести в режим стабилизации I)
					I_over=1; //Заставить снижаться ток, чтобы перейти в режим стабилизации тока
					}
				if (U_under)
					{
					if (P_wdU<MAX_CK) P_wdU++;
					PWM_set=0;
					}
				}
			if (I_over)//Если ток больше заданного
				{
				if (err_I>128)	Correct_PWM(0x01); //Если ошибка по I очень большая, то резко снизить I
				if (P_wdI>0) P_wdI--;
				PWM_set=0;
				}
			if (U_over)//Если напряжение больше заданного
				{
				if (err_U>128)	Correct_PWM(0x02); //Если ошибка по U очень большая, то резко снизить U
				if (P_wdU>0) P_wdU--;
				PWM_set=0;
				}
			}//(PWM_status==charge)
		//--------------------корректирование тока и напряжения при разряде--------------------
		if (PWM_status==discharge)  //Если  ШИМ разряда запущен
			{		
			if ((Id_under)&&(U_over))	//Если ток меньше и напряжение больше заданного
				{
				if (P_wdI<max_pwd_Id) P_wdI++;
				PWM_set=0;
				}
			if ((Id_over)||(U_under)) //Если ток больше или напряжение меньше заданного
				{
				if (Id_over)
					{
					if ((err_I>256)&&(U_over))	Correct_PWM(0x04);//Если резко уменьшили допустимый ток при этом напряжяние выше дрпустимого, то провести быструю корректировку ШИМ
					}
				if (P_wdI>0) P_wdI--;
				PWM_set=0;
				}
			if (Clb.id.bit.control) //если установлен флаг необходимости контроля калибровки тока разряда
				{
				if (U_over) //если напряжение достаточное для того чтобы выйти на стабилизацию тока (аккумулятор исправный и не "просел")
					{
					if (err_I>(Id_0t2A)) {Clb.id.bit.error=1;} //если есть ошибка по току,  то установить флаг ошибки калибровки
					else {Clb.id.bit.error=0;}   //если ошибки не большая , то установить флаг что калибровка блока впорядке
					if (Cfg.dmSlave > 0)
						{if (err_I>(Id_0t1A+ADC_ADS1118[ADC_DI].word/100*5)) Clb.id.bit.save=1;} //если ошибка по току слишком большая то установить флаг необходимости сохранения коэфициентов в EEPROM
					else
						{if (err_I>(Id_0t2A)) Clb.id.bit.save=1;} //если ошибка по току слишком большая то установить флаг необходимости сохранения коэфициентов в EEPROM
					Clb.id.bit.control=0; //установить флаг, что контроль калибровки проведён
					}
				}
			if (Clb.id.bit.error) //если есть ошибка в калибровке РМ
				{
				if ((Id_under==0)&&(Id_over==0)) //если ток застабилизировался на заданном уровне
					{
					if (limit_Id<id_dw_Clb) //если ток допустимый для сохранения калибровки на малых токах
						{
						Clb.setI1=limit_Id;//set_Id;
						Clb.pwm1=P_wdI;
						Clb.id.bit.dw_Fin=1;
						max_pwd_Id=calculate_pwd((max_set_Id+(max_set_Id/10)), 0);
						}
					if (limit_Id>id_up_Clb)//если ток допустимый для калибровки на больших токах
						{
						Clb.setI2=limit_Id;//set_Id;
						Clb.pwm2=P_wdI;
						Clb.id.bit.up_Fin=1;
						max_pwd_Id=calculate_pwd((max_set_Id+(max_set_Id/10)), 0);
						}
					}
				}
			}//(PWM_status==discharge)
		}//if (correct_off==0)
//----вычисление времени следующей корректировки в зависимости от ошибки------------
	if (PWM_status!=0)
		{
		ADC_cfg_wr.bit.DR=7;
		if (min_error<2048) ADC_cfg_wr.bit.DR=6;
		if (min_error<1024) ADC_cfg_wr.bit.DR=5;
		if (min_error<512) ADC_cfg_wr.bit.DR=4;
		if (min_error<256) ADC_cfg_wr.bit.DR=3;
		if (min_error<128) ADC_cfg_wr.bit.DR=2;
		if (min_error<64) ADC_cfg_wr.bit.DR=1;
		if (min_error<32) ADC_cfg_wr.bit.DR=0;
	
		/*if (PWM_status==discharge)
			{if (P_wdI==0) ADC_cfg_wr.bit.DR=0;}
		else
			{if ((P_wdU==0)||(P_wdI==0)) ADC_cfg_wr.bit.DR=0;}*/
		}
	else
		{
		ADC_cfg_wr.bit.DR=0;//Если ШИМ не запущен, тогда корректировка не нужна, нужно только среднее значение АЦП
		}
	}//	if ((ADS1118_St[ADC_MU]==1)&&((ADS1118_St[ADC_MI]==1)||(ADS1118_St[ADC_DI]==1))) //если оцифрованы одновременно: (U и I) или (U и Id)
}
#endif
//-------------------------------------------------------------------------------------
unsigned int i_power_limit(unsigned int p, unsigned int i)
{uint32_t U, Id;
	p_limit=0;
	//U=adc_to_value(ADC_ADS1118[ADC_MU].word, ku_cfg.K[ADC_MU], 0, BASE_D)/1000000UL;
	U=(((uint32_t)ADC_ADS1118[ADC_MU].word)*((uint32_t)Cfg.K_U))/1000000UL;	
	if (U>0)
		{
		//Id=value_to_adc((((uint64_t)p*10000000ULL)/(uint64_t)U), ku_cfg.K[ADC_DI], 0, BASE_D);
		Id=(((uint64_t)p*10000000ULL)/(uint64_t)U)/Cfg.K_Id;
		if ((Id<32768)&&(Id>0))
			{
			if (i>(uint16_t)Id) 
				{
				i=Id; 
				p_limit=1;
				}	//если превышена мощность, то снизить ток и установить флаг ограничения мощности
			}
		}
	return(i);
}
//-------------------------------------------------------------------------------------
void Err_check(void)
{
//---------Проверка ошибки по внешнему входу----------
	if ((CSU_cfg.bit.LED_ON==0)&&(CSU_cfg.bit.EXT_Id==1)) //если индикация не светодиодная и включено управление вншним разрядным модулем
		{
		if ((OverTempExt&&1)!=(CSU_cfg.bit.EXTt_pol&&1)) ERR_Ext++;//проверка срабатывания внешнего термореле
		else ERR_Ext=0;
		
		if (ERR_Ext>100) Error=ERR_OVERTEMP3;
		
		/*if (CSU_cfg.bit.DIAG_WIDE) //Если включена расширеная диагностика
			{
			//if (ADC_O[ADC_MI]<(B_I_const+(B_I_extId>>2))) Error=ERR_DM_LOSS; //Если ток, потребляемый разрядным модулем стал в 4 раза меньше расчётного, то обрыв РМ
			if (ADC_O[ADC_MI]<(B[ADC_MI]>>1)) Error=ERR_DM_LOSS; //Если ток, потребляемый разрядным модулем стал в 4 раза меньше расчётного, то обрыв РМ
			else if (Error==ERR_DM_LOSS) Error=0;
			}*/
		}
//---------Проверка на обрыв РМ-----------------------
	if (Cfg.dmSlave>0)
		{
		if ((CSU_cfg.bit.DIAG_WIDE)&&(PWM_status==DISCHARGE)) //Если включена расширеная диагностика
			{
			if ((dm_loss_cnt>0)&&(dm_loss_cnt<10))	
				{
			//	if ((set_Id<<1)<ADC_ADS1118[ADC_DI].word) //если реальный ток в 2 раза больше заданного
			//		{
			//		Error=ERR_DISCH_PWR;
			//		}	
				if (ADC_ADS1118[ADC_DI].word>Id_A(0,1))
					{
					if ((set_U<ADC_ADS1118[ADC_MU].word)&&(p_limit==0)&&(set_Id>(ADC_ADS1118[ADC_DI].word<<1)))
						Error=ERR_DM_LOSS;
					if ((set_Id>(ADC_ADS1118[ADC_DI].word+Id_A(0,2)))&&(P_wdI==max_pwd_Id))//если реальный ток в 2 раза меньше заданного
						Error=ERR_DM_LOSS;
					}
				}
			}
		}
//---------Проверка на обрыв нагрузки-----------------
	if ((CSU_cfg.bit.I0_SENSE)&&(CSU_cfg.bit.GroupM==0)) //Если установлена диагностика обрыва нагрузки и блок работает не в группе
		{
		if ((PWM_status==CHARGE)&&(set_I!=0))
			{
			if (ADC_ADS1118[ADC_MI].word>=Id_A(0,1)) No_akb_cnt=70; //проверка отсутвия АКБ
			if ((No_akb_cnt==0)&&(P_wdI>0))	Error=ERR_NO_AKB;
			}
		if ((PWM_status==DISCHARGE)&&(set_Id!=0))
			{
			if (ADC_ADS1118[ADC_DI].word>=Id_A(0,1)) No_akb_cnt=70; //проверка отсутвия АКБ
			if ((No_akb_cnt==0)&&(P_wdI>0))	Error=ERR_NO_AKB;
			}
		}

//---------Проверка перегрева----------------------------
	if (PWM_status==DISCHARGE)
		{
		if ((Temp2.fld.V>MAX_T2dch)&&(Temp2.word!=0xFFFF)) Error=ERR_OVERTEMP2; //проверка перегрева разрядных транзисторов
		}
	if (PWM_status==CHARGE)
		{
		if ((Temp1.fld.V>MAX_T1)&&(Temp1.word!=0xFFFF)) Error=ERR_OVERTEMP1; //проверка перегрева транзисторов
		if ((Temp2.fld.V>MAX_T2ch)&&(Temp2.word!=0xFFFF)) Error=ERR_OVERTEMP2; //проверка перегрева выпрямительных диодов
		}

//---------Проверка полярности подключения АКБ (переполюсовка)-
	if (CSU_cfg.bit.GroupM==0)
		{
	/*	if (!Good_Con) //проверка неверного подключения АКБ
			{
			Error=ERR_CONNECTION1;
			}
		else {if (Error==ERR_CONNECTION1) Error=0;}*/

		if (ADC_O[ADC_MU]<B[ADC_MU])
			{
			if ((B[ADC_MU]-ADC_O[ADC_MU])>300) Error=ERR_CONNECTION;
			else {if (Error==ERR_CONNECTION) Error=0;}
			}
		else {if (Error==ERR_CONNECTION) Error=0;}
		}

//---------Проверка несиправности ЗРМ: неисправность АЦП, неисправность выпрямителя-----
if (CSU_cfg.bit.DIAG_WIDE)
	{
	if ((PWM_status==STOP)&&(CSU_cfg.bit.GroupM==0)) //Если преобразователь выключен и блок работает не в группе
		{
		if ((ADS1118_St[ADC_MUp]!=0)&&(ADS1118_St[ADC_MU]!=0)) //Если есть данные о напряжении
			{
			if ((ADC_ADS1118[ADC_MU].word>U_V(0,8))&&(ADC_ADS1118[ADC_MUp].word<U_V(0,2))) {if (OUT_err_cnt<250) OUT_err_cnt++;}
			else OUT_err_cnt=0;
			
			if (OUT_err_cnt>2) Error=ERR_OUT; //если на выходе после реле напряжение есть, а до реле напряжения нет, значит КЗ выхода
			if ((OUT_err_cnt==0)&&(Error==ERR_OUT)) Error=0;
//			else {if (Error==ERR_OUT) Error=0;}
			ADS1118_St[ADC_MU]=0;
			ADS1118_St[ADC_MUp]=0;
			}
			
		/*if ((ADS1118_St[ADC_MI]!=0)&&(CSU_cfg.bit.GroupM==0)) //Если оцифрованы данные тока
			{
			if (ADC_ADS1118[ADC_MI].word>I_1A) {if (Err_ADC_cnt<10) Err_ADC_cnt++;} //Если преобразователь выключен, но при этом есть показания тока
			else Err_ADC_cnt=0;	
			ADS1118_St[ADC_MI]=0;
			}
		if (Err_ADC_cnt>2) Error=ERR_ADC; //Если несколько раз подряд присутствовали данные тока при выключеном ШИМ*/
		if ((ADC_cfg_rd.word==0)||(ADC_cfg_rd.word==0xFFFF)) Error=ERR_ADC; //Если данные конфигурации АЦП неверные значит АЦП неисправен
		}
	
	if (ADC_wait==0) Error=ERR_ADC; //Если долгое вермя не удаётся прочитать значение АЦП
	}
//---------Проверка перегрузки
	if (Overload) Error=ERR_OVERLOAD; //проверка защиты от перегрузки
	if (Error) //Если есть ошибка 
		{
			if (PWM_status!=STOP) Stop_PWM(0); //если преобразователь не выключен: выключить преобразователь
			if (RELAY_EN)
			{
				RELAY_OFF; //Если реле не разомкнуто, разомкнуть реле
				ALARM_OUT(1);
				alarm_del = 0;
			}
		}//*/
}
//-------------------------------------------------------------
void Start_CSU(unsigned char mode)
{unsigned char control_setU=0;

Err_check();
if (Error) return;

#if PID_CONTROL
if (!CSU_Enable) {
  PulseMode = true;
  init_PIDs();
  State = RESET_ST;
}
#endif

if ((CSU_Enable==0)&&(set_U!=0)) control_setU=1; //Если запускаем из состояния "стоп", при этом заданное напряжение не 0В
else control_setU=0;

if (PWM_status!=0) 
	{
	Stop_PWM(1);
	p_limit=0;
	}
CSU_Enable=mode&0x0F;

if (CSU_Enable==PAUSE) {
	if (RELAY_EN!=0) {
		RELAY_OFF; //Выключить реле
		ALARM_OUT(1);
		alarm_del = 0;
	}
	return;
}

if (CSU_Enable==DISCHARGE) DE(1);
if (RELAY_EN==0)
	{
	RELAY_ON; //Включить реле
	delay_ms(100);	
	}

if (CSU_Enable==CHARGE)
	{
#if !PID_CONTROL
	if ((CSU_cfg.bit.LED_ON)&&(connect_st==0))
		{
		if (ADC_ADS1118[ADC_MU].word>U_V(16,0)) set_U=U_V(31,0);
		else set_U=U_V(16,0);
		set_I=I_A(1,0);
		}
#endif
	soft_start(CSU_cfg.bit.DIAG_WIDE&&control_setU&&(CSU_cfg.bit.GroupM==0));
	ADC_ADS1118[ADC_MI].word=0;
	}
if (CSU_Enable==DISCHARGE)
	{
	soft_start_disch();	
	ADC_ADS1118[ADC_DI].word=0;
	}
No_akb_cnt=70;
}
//-------------------------------------------------------------
void Stop_CSU(unsigned char mode) {
	if (Error==0) autosrart.restart_cnt=AUTOSTART_CNT;
	Error=0;
	Stop_PWM(1);
	p_limit=0;
	CSU_Enable=0;
	delay_ms(10);
	if (mode==0) {
		RELAY_OFF; //Выключить реле
		ALARM_OUT(1);
		alarm_del = 0;
	} else {
		RELAY_ON; //Включить реле
	}
}
//-------------------------------------------------------------
void Read_temp(void)
{Temp_type Temp1_v, Temp2_v;

Err_Thermometr=Read_Current_Temperature(&Temp1_v, &Temp2_v);
//-------датчик N1
if (Err_Thermometr&Tmask1)
	{
	if (Err1_cnt<READ_ERR_CNT) Err1_cnt++;
	else Temp1.word=0xFFFF;
	}
else
	{
	Err1_cnt=0;
	Temp1.word=Temp1_v.word;
	}
//-------датчик N2
if (Err_Thermometr&Tmask2)
	{
	if (Err2_cnt<READ_ERR_CNT) Err2_cnt++;
	else Temp2.word=0xFFFF;
	}
else
	{
	Err2_cnt=0;
	Temp2.word=Temp2_v.word;
	}

Err_Thermometr=Thermometr_Start_Convert();
}

//-------------------------------------------------------------
void update_LED(void)
{
/*if (Error||Err_Thermometr)
	{
	if ((Error==ERR_OVERLOAD)||(Error==ERR_OUT)||(Error==ERR_ADC)) LED_ERR(1);//LED&=0x7F;
	}
else LED_ERR(0);//LED|=0x80;*/

if (Error==ERR_CONNECTION) LED_POL(1);
else LED_POL(0);//LED|=0x08;
	
if (CSU_Enable!=0) LED_PWR(1);//LED&=0xBF;

if (PWM_status==CHARGE)
	{
	if (I_St) {LED_STI(1);LED_STU(0);}//LED=LED&0xDF|0x10;
	else	  {LED_STI(0);LED_STU(1);}//LED=LED&0xEF|0x20;
	}
else
	{LED_STI(0);LED_STU(0);}//LED|=0x30;
}
//===============================================Прерывание таймера/счетчика Т0==============================================
#pragma vector=TIMER0_OVF_vect
#pragma type_attribute=__interrupt
void tmr0_ovf(void)
{
//------------------------Параметры зависящие от времени----------------
if (pid_time) pid_time--;
if (connect_st>0) connect_st--; //Время ожидания следующего пакета (по истечении вермени связь считается разорваной)
if (No_akb_cnt>0) No_akb_cnt--; //Время ожидания пока нарастёт выходной ток (если время истекло, а ток не вырос, значит АКБ не подключена)
if (time_rd>0) time_rd--;		//Время в течении которого запрещено чтение датчика температуры (пауза между чтениями датчика температуры)
if (fast_correct>0) fast_correct--; //Время в течении которого запрещена быстрая смена ШИМ
if (correct_off>0) correct_off--;	//Время в течении которого запрещена любая смена ШИМ
if (ADC_wait>0) ADC_wait--;	//Максимально допустимое время ожидания оцифровки для АЦП
if (dm_loss_cnt>0) dm_loss_cnt--; //Задрежка после пуска перед проверкой обрыва РМ

if ((Cfg.bf2.ast)&&(PWM_status==STOP))
	{
	if (autosrart.restart_time>0) autosrart.restart_time--;	
	}
	
if ((connect_st==0)||(CSU_cfg.bit.DEBUG_ON==1))
	{
	if (Key_delay>0) Key_delay--;   //Время в течении которого запрещено сканирование клавиш (пауза после нажатия клавиши)
	if (KeyPress>0) KeyPress--;		//Время удержания клавиши непрерывно нажатой
	
//------------------------Параметры с LCD--------------------------------
	if ((CSU_cfg.bit.LCD_ON)||(CSU_cfg.bit.DEBUG_ON))
		{
		if (LCD_refresh>0) LCD_refresh--;//Время в течении которго запрещено обновление LCD (пауза между обновлениями LCD)
		if (fCnt>0) fCnt--; //время до перехода на следующий этап
		if (pulse_step>0) pulse_step--; //время смены импульса заряд/разряд в импульсном режиме
		if ((Stg.fld.stop_flag.dU)&&(dU_time>0)) dU_time--; //Время когда не увеличивается U при заряде щелочного АКБ
	
	
		if (PWM_status!=0) 
			{ 
			if (mSec>59) 
	 			{
				Sec++;			//секунды метода
				Sec_Stg++;	//секунды этапа
				mSec=0;
				}
			else mSec++;	
			if (Sec>59)	
	 			{
				Min++; //минуты метода
				Sec=0;
				}
			if (Min>59)	
	 			{
				if (Hour<255) Hour++;	//Часы метода
				Min=0;
				}
			if (Sec_Stg>59)
				{
				Min_Stg++;	//Минты этапа
				Sec_Stg=0;
				}
			if (Min_Stg>59)
				{
				Hour_Stg++;	//Часы этапа
				Min_Stg=0;
				}
			}
		}

//------------------------Индикация ошибок для LED-------------------------	
	if (CSU_cfg.bit.LED_ON)
		{
		if (mSec>59) mSec=0;
		else mSec++;
		
		/*if (Error==ERR_NO_AKB)
			{
			if (mSec<10) LED_ERR(0);//PORTC=PINC|0x80;
			else LED_ERR(1);//PORTC=PINC&0x7F;
			}*/
		/*if ((Error==ERR_OVERTEMP1)||(Error==ERR_OVERTEMP2)||(Err_Thermometr!=0))
			{
			if (mSec==29) LED_ERRinv;//PORTC=PINC^0x80;
			}*/
		if (CSU_Enable==0)
			{
			if ((mSec<10)||((mSec>20)&&(mSec<30))) LED_PWR(1);//PORTC=PINC&0xBF;
			else LED_PWR(0);//PORTC=PINC|0x40;
			}	
		}//if (CSU_cfg.bit.LED_ON)	
	}//if (connect_st==0)
 //TIFR=0;
}

//===================================================Прерывание 1 внешнее====================================================
//ISR(INT1_vect)
//-------------------------------------------------------------
void Init_Timer0(void)
{
TCCR0 =(1<<CS00)|(1<<CS02); // clk=CK/1024
//TCNT0 = Timer0_value;
TIMSK=(1<<TOIE0);// enable overflow interrupt
}
//-------------------------------------------------------------
void Init_ExtInt(void)
{
MCUCR=(1<<ISC00)|(0<<ISC01)|(1<<ISC10)|(0<<ISC11); //CSU Rev2 //interrup1 on  any changing
GICR=(1<<INT1);
}
//-------------------------------------------------------------
static void init_gpio (void) {
    DDRA = 0x07;
    DDRB = 0xB8;
#if !JTAG_DBGU
    DDRC = 0xFF;
    PORTC = 0xFF;
#endif
    DDRD = 0xF4;
}