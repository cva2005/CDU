#pragma message	("@(#)main.c")

#include <system.h>
#include "pid/pid_r.h"
#include "net/net.h"
#include "pwm/pwm.h"
#include "csu/csu.h"
#include "csu/mtd.h"
#include "tsens/ds1820.h"
#include "lcd/lcd.h"
#include "key/key.h"
#include "spi/adc/ads1118.h"

unsigned int dU_time;	//время при котором напряжение неизменно в заряде щелочного АКБ
unsigned int fast_correct=0; //таймаут запрета на быструю коррекцию ШИМ
unsigned char correct_off=0, change_UI=0; //запрет коррекции ШИМ, заданные значения тока и напряжения изменились

static inline void init_gpio (void);

void main (void) {
    init_gpio();
    ALARM_OUT(0);
    read_cfg();
    if (Cfg.bf1.LCD_ON) {
        lcd_wr_connect(false);
        lcd_wr_set();
    }
    init_rs();
    SYS_TMR_ON();
    START_RX();
    __enable_interrupt();
    adc_init();
    // ToDo: use WDT?
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
        csu_drv();
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
	if (KeyDel>0) KeyDel--;   //Время в течении которого запрещено сканирование клавиш (пауза после нажатия клавиши)
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
		/*if ((Error==ERR_OVERTEMP1)||(Error==ERR_OVERTEMP2)||(ErrTmp!=0))
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
