#include "../sys/system.h"
#include "../csu/csu.h"
#include "../lcd/wh2004.h"
#include "../spi/adc/ads1118.h"
#include "pwm.h"

extern int TEST1, TEST2, TEST3, TEST4;

extern calibrate_t calibrate;
extern unsigned char PWM_status, Error;
extern ADC_Type  ADC_ADS1118[4];
extern unsigned char ADS1118_St[4]; //состояние обработаны данные или нет
extern unsigned char ADC_finish;
extern unsigned int set_I, set_U, set_Id;
extern unsigned int K_U, K_I, K_Id;
extern unsigned int P_maxW;

extern CSU_type CSU_cfg;
extern unsigned char DM_SLAVE;

extern unsigned int max_pwd_I, max_pwd_U, max_pwd_Id;
extern unsigned int fast_correct;
extern unsigned char correct_off, change_UI;
extern unsigned char PWM_set;

extern unsigned int dm_loss_cnt;
//-------------------------------------------------------------
unsigned short PwmDuty (float out) {
  if (out > 1.0f) out = 1.0f;
  if (out < 0) out = 0;
  out *= MAX_CK;
  return (unsigned short)out;
}
//-------------------------------------------------------------
void Stop_PWM(unsigned char soft)
{
if (soft!=0)
	{
	while ((P_wdI>0)&&(P_wdU>0))
		{
		if (P_wdI>0) P_wdI--;
		if (P_wdU>0) P_wdU--;
		 delay_us(10);
		}
	P_wdU=0;
	P_wdI=0;	
	delay_ms(10);
	}

PWM_ALL_STOP;

TCCR1A = 0x00; 
TCCR1B = 0x00; //stop
P_wdU=0;
P_wdI=0;
ICR1=0;
TCNT1=0;

delay_ms(10);
PWM_status=0; //установить признак что PWM не работает
PWM_set=0;    //Установить признак что значения тока и напряжение не застабилизированы
//устанвоить признак что значения АЦП не актуальны
ADS1118_St[ADC_MU]=0;
ADS1118_St[ADC_MI]=0;
ADS1118_St[ADC_DI]=0;
ADS1118_St[ADC_MUp]=0;
return;
}

//-------------------------------------------------------------
void Start_PWM_T1(unsigned char mode) {
  if (PWM_status!=0) Stop_PWM(0);
  ICR1 = MAX_CK; //макс. значение счётчика для режима PWM Frecuency Correct:ICR1;	 
  P_wdU = P_WDU_start; //Задать ширину импульса для канала А
  P_wdI = P_WDI_start; //Задать ширину импульса для канала Б
  TCCR1A = (1 << COM1B1) | (1 << WGM11); // OC1A отключен , OC1B инверсный, режим FAST PWM:ICR1.
  if (mode == charge) TCCR1A |= 1 << COM1A1; // OC1A инверсный
  TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS10); //0x11; //CK=CLK ,режим FAST PWM:ICR1	
  //delay_us(10);
  PWM_status = mode; //установить признак что PWM работает в режиме заряда
}

//-------------------------------------------------------------
void soft_start(unsigned char control_out)
{

Start_PWM_T1(charge);			//запустить преобразователь

if (control_out)
	{
	if (ADC_ADS1118[ADC_MU].word>set_U) Error=ERR_SET;
	}

if (!Error)
	{
#if !PID_CONTROL
  uint32_t S=(uint32_t)set_I*100UL/K_PWD_I;
	P_wdI=((unsigned int)S) + B_PWD_I;
	if (P_wdI>max_pwd_I) P_wdI=max_pwd_I;
	
	S=(uint32_t)set_U*100UL/K_PWD_U;
	P_wdU=((unsigned int) S) + B_PWD_U;
	if (P_wdU>max_pwd_U) P_wdU=max_pwd_U;
#endif
	SD(1);
	}
ADS1118_St[ADC_MU]=0;
ADS1118_St[ADC_MI]=0;
ADC_finish=0;
correct_off=22; //запретить корректировку ШИМ, пока не стабилизируется напряжение
change_UI=0;
}

//-------------------------------------------------------------
unsigned int calculate_pwd(unsigned int val, unsigned char limit)
{int32_t P=0L, K, B;

	K=(1000L*(calibrate.setI2-calibrate.setI1))/(calibrate.pwm2-calibrate.pwm1); //рассчитать зхначения K
	B=calibrate.pwm1-((int32_t)calibrate.setI1*1000L/K); //рассчитать зхначения B
	P=(int32_t)val*1000L/K+B;
	if (P<0) P=0;
	if (P>MAX_CK) P=MAX_CK;
	if (limit)
		{
		if ((unsigned int)P>max_pwd_Id) P=max_pwd_Id;
		}
	
	return((unsigned int) P);	
}

void soft_start_disch(void)
{unsigned int I;

	
Start_PWM_T1(discharge);

//#if !PID_CONTROL
I=i_power_limit(P_maxW, set_Id); //Проверка превышения общей мощности
P_wdI=calculate_pwd(I, 1);
P_wdU=0;
//#endif
ADS1118_St[ADC_MU]=0;
ADS1118_St[ADC_DI]=0;
ADC_finish=0;
correct_off=20; //запретить корректировку ШИМ, пока не стабилизируется напряжение
change_UI=0;
calibrate.id.bit.control=1; //установить флаг для калибровки: проконтролировать калибровку
dm_loss_cnt=100;
}

//-------------------------------------------------------------
#if !PID_CONTROL
void Correct_PWM(unsigned char pr)
{uint32_t S=0UL;
 unsigned int I_p=0, U_p=0, limit_id;
 
if (fast_correct) return;

if (PWM_status==charge)
	{
	if (pr&0x01)
		{
		S=(uint32_t)set_I*100UL/K_PWD_I;
		I_p=((unsigned int) S)+B_PWD_I;
		if ((P_wdI>I_p)&&((pr&0x10)!=0x10)) I_p+=(B_PWD_I>>1); //Если снижаем ток свеху вниз, при этом разрешено поставить немного завышеный ток
		//if (P_wdI>I_p) I_p+=(B_PWD_I>>1); //Если снижаем ток свеху вниз, при этом разрешено поставить немного завышеный ток
		//if (pr&0x10) I_p-=(B_PWD_I<<1); //Если снижаем ток свеху вниз, при этом разрешено поставить немного завышеный ток
		if (I_p>max_pwd_I) I_p=max_pwd_I;

		while (I_p!=P_wdI)
			{
			if ((P_wdI<I_p)&&(P_wdI<MAX_CK)) P_wdI++;
			if ((P_wdI>I_p)&&(P_wdI>0)) P_wdI--;
			delay_us(10);
			}
		}
	
	if (pr&0x02)
		{
		S=(uint32_t)set_U*100UL/K_PWD_U;
		U_p=((unsigned int) S)+B_PWD_U;
		if ((P_wdU>U_p)&&((pr&0x20)!=0x20)) U_p+=(B_PWD_U>>1); //Если снижаем напряжение свеху вниз, при этом разрешено поставить немного завышеное напряжение
		//if (P_wdU>U_p) U_p+=(B_PWD_U>>1); //Если снижаем напряжение свеху вниз, при этом разрешено поставить немного завышеное напряжение
		//if (pr&0x20) U_p-=(B_PWD_U>>1); //Если снижаем напряжение свеху вниз, при этом разрешено поставить немного завышеное напряжение
		if (U_p>max_pwd_U) U_p=max_pwd_U;
		
		while (U_p!=P_wdU)
			{
			if ((P_wdU<U_p)&&(P_wdU<MAX_CK)) P_wdU++;
			if ((P_wdU>U_p)&&(P_wdU>0)) P_wdU--;
			delay_us(10);
			}
		}
	}
	
if (PWM_status==discharge)
	{
	limit_id=i_power_limit(P_maxW, set_Id); //Проверка превышения общей мощности
	if ((P_wdI>limit_id)&&((pr&0x40)!=0x40)) limit_id+=(Id_0t1A); //Если снижаем ток свеху вниз, при этом разрешено поставить немного завышеный ток
	I_p=calculate_pwd(limit_id, 1);
	
	while ((I_p!=P_wdI)&&(P_wdI!=max_pwd_Id)&&(P_wdI!=0))
		{
		if ((P_wdI<I_p)&&(P_wdI<max_pwd_Id)) P_wdI++;
		if ((P_wdI>I_p)&&(P_wdI>0)) P_wdI--;
		delay_us(10);
		}
	}

ADS1118_St[ADC_MU]=0;
ADS1118_St[ADC_MI]=0;
ADS1118_St[ADC_DI]=0;
ADC_finish=0;
fast_correct=400; //Запретить на 3c. быструю корректировку шим
change_UI=0;
}
#endif