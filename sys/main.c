#pragma message	("@(#)main.c")

#include <system.h>
#include "pid/pid_r.h"
#include "net/net.h"
#include "pwm/pwm.h"
#include "csu/csu.h"
#include "csu/mtd.h"
#include "tsens/ds1820.h"
#include "lcd/wh2004.h"
#include "lcd/lcd.h"
#include "key/key.h"
#include "spi/adc/ads1118.h"

int TEST1=0, TEST2=0, TEST3=0, TEST4=0;
bool SelfCtrl = false; //управление методом заряда производится самостоятельно или удалённо
unsigned int pulse_step; //время импульса заряд/разряд при импульсном режиме
unsigned int dU_time;	//время при котором напряжение неизменно в заряде щелочного АКБ
unsigned int fast_correct=0; //таймаут запрета на быструю коррекцию ШИМ
unsigned char correct_off=0, change_UI=0; //запрет коррекции ШИМ, заданные значения тока и напряжения изменились
unsigned char time_rd=0; //время между чтениями датчиков температуры

/*-#define NO_BATT_TIME    70
#define INF_TAU         50.0f
#define CURR_DT         100.0f
#define ST_TIME         100U
#define DOWN_LIM        100.0f*/

static inline void init_gpio (void);
static inline void fan_ctrl (void);
static inline void check_key_press (void);

void main (void) {
    init_gpio();
    ALARM_OUT(0);
    Read_temp(); //Запустить на измерение датчик температуры 
    read_cfg();
    if (Cfg.bf1.LCD_ON) {
        LCD_wr_connect(0);
        LCD_wr_set();
    }
    init_rs();
    SYS_TMR_ON();
    START_RX();
    __enable_interrupt();
    adc_init();
    if (Cfg.bf1.FAN_CONTROL == 0) {
        FAN(1);
        delay_ms(500);
        delay_ms(500);
        read_mtd();
        FAN(0);
    }
    while (true) {	  	
        net_drv();
        adc_drv();
        csu_time_drv();
        err_check();
        if (Read_ADS1118(&ADS1118_chanal)) { //Если есть оцифрованные каналы
            if (!Cfg.bf1.LED_ON || RsActive) Correct_UI();
        }
        if (Clb.id.bit.save == 1) {
            if (!Clb.id.bit.error && (Clb.id.bit.dw_Fin || Clb.id.bit.up_Fin)) {
                save_clb();
                Clb.id.byte = 0;
            }
        }
        fan_ctrl();
        if (Cfg.bf1.LCD_ON) {
            if (!RsActive) { //если подключение прервалось, а значения дисплея не обновлены
                if (LCD[0][0]!='P') {
                    if (Cfg.bf1.DIAG_WIDE) {
                        Stop_CSU(STOP);
                        read_mtd();
					}
                    LCD_wr_set();
				}				
			} else	{ //если подключение появилось, а значения дисплея не обновлены
                if (!Cfg.bf1.DEBUG_ON) {
                    if (LCD[1][0] != P_rus) LCD_wr_connect(1);
                }
            }			
        }
        if (!RsActive || Cfg.bf1.DEBUG_ON) {
            if (Cfg.bf1.LED_ON)	update_LED();
            if (Cfg.bf1.LCD_ON) {
                if (!LCD_refresh) {
                    update_LCD_work();
                    if (!CsuState) LCD_refresh = 25;
                }
            }
            if (CsuState && SelfCtrl) { //Если идёт заряд или разряд и управление зарядом осуществляет ЗРМ самостоятельно
                stg_status(); //проверить статус этапа: испульсный режим или нет, в случае необходимости изменить исмпульс
			}
            check_key_press();
            check_auto_start();
        }
		if (ALARM_ON()) { // отключение сигнала окончания работы
 			if (!get_time_left(AlarmDel)) {
				ALARM_OUT(0);
			}
		}
	}
}

unsigned int i_power_limit(unsigned int p, unsigned int i) {
    uint32_t U, Id;
	pLim = false;
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
				pLim = true;
				}	//если превышена мощность, то снизить ток и установить флаг ограничения мощности
			}
		}
	return(i);
}

void Read_temp(void)
{Temp_type Temp1_v, Temp2_v;

Err_Thermometr=Read_Current_Temperature(&Temp1_v, &Temp2_v);
//-------датчик N1
if (Err_Thermometr&Tmask1)
	{
	if (Err1_cnt<READ_ERR_CNT) Err1_cnt++;
	else Temp1.word=ERR_WCODE;
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
	else Temp2.word=ERR_WCODE;
	}
else
	{
	Err2_cnt=0;
	Temp2.word=Temp2_v.word;
	}

Err_Thermometr=Thermometr_Start_Convert();
}

void update_LED(void)
{
/*if (Error||Err_Thermometr)
	{
	if ((Error==ERR_OVERLOAD)||(Error==ERR_OUT)||(Error==ERR_ADC)) LED_ERR(1);//LED&=0x7F;
	}
else LED_ERR(0);//LED|=0x80;*/

if (Error==ERR_CONNECTION) LED_POL(1);
else LED_POL(0);//LED|=0x08;
	
if (CsuState!=0) LED_PWR(1);//LED&=0xBF;

if (PwmStatus==CHARGE)
	{
	if (I_St) {LED_STI(1);LED_STU(0);}//LED=LED&0xDF|0x10;
	else	  {LED_STI(0);LED_STU(1);}//LED=LED&0xEF|0x20;
	}
else
	{LED_STI(0);LED_STU(0);}//LED|=0x30;
}

static inline void check_key_press (void) {
    uint8_t key_n;
    if (scan_key(&key_n)) { //если есть нажатая кнопка
        if (Key_delay==0) { //если разрешена обработка кнопок
            if (Cfg.bf1.LCD_ON) {
                if (!KeyPress) { //Если долгое время клавишу удерживали нажатой
                    KeyPress = 200;
                    if (Step==1) Step = 10;
                    else Step=20;
                }
                if (key_n==K5) key_power();//если нажата кнопка Старт/Стоп
                if (key_n==K4) key_set();//если нажата кнопка Set
                if (key_n==K3) key_up(); //если нажата кнопка вверх
                if (key_n==K2) key_dw(); //если нажата кнопка вниз		
            }
            if (Cfg.bf1.LED_ON) {
                if (!KeyPress) { //Если долгое время клавишу удерживали нажатой
                    KeyPress = 100;
                    if (Step == 1) Step=5;
                    else Step=10;
                }
                if (key_n == K5) key_power_LED();//если нажата кнопка Старт/Стоп
                if (key_n == K4) key_U_up();//если нажата кнопка Set
                if (key_n == K3) key_U_dw(); //если нажата кнопка вверх
                if (key_n == K2) key_I_up(); //если нажата кнопка вниз	
                if (key_n == K1) key_I_dw(); 
            }
        }
    } else { //если нет нажатой кнопки
        KeyPress=100;
        Step=1;
    }
}

static inline void fan_ctrl (void) {
    if (!time_rd) { //Если пришло время прочитать датчик температуры
        time_rd = 30; //Установить время следующего чтения датчика
        Read_temp(); //Прочитать датчики температуры
        if (!Cfg.bf1.FAN_CONTROL) { //Если запрещено внешнее управление вентиляторами
            if ((Temp1.fld.V > FAN_ON_T) || (Temp2.fld.V > FAN_ON_T) ||
                ((Temp1.fld.V >= - FAN_CND_T) && (Temp1.fld.V < FAN_CND_T)) ||
                ((Temp2.fld.V >= -FAN_CND_T) && (Temp2.fld.V < FAN_CND_T)) ||
                (Temp1.fld.D == 0xFF) || (Temp2.fld.D == 0xFF)) {
                FAN(1);
            } else {
                if ((Temp1.fld.V < FAN_OFF_T) && (Temp2.fld.V < FAN_OFF_T)) FAN(0);
            }
        }
    }
}

static inline void init_gpio (void) {
    DDRA = 0x07;
    DDRB = 0xB8;
#if !JTAG_DBGU
    DDRC = 0xFF;
    PORTC = 0xFF;
#endif
    DDRD = 0xF4;
    MCUCR = (1 << ISC00) | (1 << ISC10);
    GICR = 1 << INT1; // ext int
}

//===============================================Прерывание таймера/счетчика Т0==============================================
#if 0
#pragma vector=TIMER0_OVF_vect
#pragma type_attribute=__interrupt
void tmr0_ovf(void)
{
//------------------------Параметры зависящие от времени----------------
if (pid_time) pid_time--;
if (connect_st>0) connect_st--; //Время ожидания следующего пакета (по истечении вермени связь считается разорваной)
if (BreakTime>0) BreakTime--; //Время ожидания пока нарастёт выходной ток (если время истекло, а ток не вырос, значит АКБ не подключена)
if (time_rd>0) time_rd--;		//Время в течении которого запрещено чтение датчика температуры (пауза между чтениями датчика температуры)
if (fast_correct>0) fast_correct--; //Время в течении которого запрещена быстрая смена ШИМ
if (correct_off>0) correct_off--;	//Время в течении которого запрещена любая смена ШИМ
if (ADC_wait>0) ADC_wait--;	//Максимально допустимое время ожидания оцифровки для АЦП
if (dm_loss_cnt>0) dm_loss_cnt--; //Задрежка после пуска перед проверкой обрыва РМ

if ((Cfg.bf2.ast)&&(PwmStatus==STOP))
	{
	if (autosrart.restart_time>0) autosrart.restart_time--;	
	}
	
if ((connect_st==0)||(Cfg.bf1.DEBUG_ON==1))
	{
	if (Key_delay>0) Key_delay--;   //Время в течении которого запрещено сканирование клавиш (пауза после нажатия клавиши)
	if (KeyPress>0) KeyPress--;		//Время удержания клавиши непрерывно нажатой
	
//------------------------Параметры с LCD--------------------------------
	if ((Cfg.bf1.LCD_ON)||(Cfg.bf1.DEBUG_ON))
		{
		if (LCD_refresh>0) LCD_refresh--;//Время в течении которго запрещено обновление LCD (пауза между обновлениями LCD)
		if (fCnt>0) fCnt--; //время до перехода на следующий этап
		if (pulse_step>0) pulse_step--; //время смены импульса заряд/разряд в импульсном режиме
		if ((Stg.fld.stop_flag.dU)&&(dU_time>0)) dU_time--; //Время когда не увеличивается U при заряде щелочного АКБ
	
	
		if (PwmStatus != STOP) 
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
	if (Cfg.bf1.LED_ON)
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
		if (CsuState==0)
			{
			if ((mSec<10)||((mSec>20)&&(mSec<30))) LED_PWR(1);//PORTC=PINC&0xBF;
			else LED_PWR(0);//PORTC=PINC|0x40;
			}	
		}//if (Cfg.bf1.LED_ON)	
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
#endif
