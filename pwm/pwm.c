#pragma message	("@(#)pwm.c")
#include <system.h>
#include "csu/csu.h"
#include "lcd/wh2004.h"
#include "spi/adc/ads1118.h"
#include "pwm.h"

uint16_t max_pwd_I = MAX_CK, max_pwd_U = MAX_CK, max_pwd_Id = 0;
csu_st PwmStatus = STOP;
uint8_t PWM_set = 0;

static void Start_PWM_T1(csu_st mode);

uint16_t PwmDuty (float out) {
    if (out >= 1.0f) return MAX_CK;
    if (out <= 0) return 0;
    out *= MAX_CK;
    return (uint16_t)out;
}

void Stop_PWM (bool soft) {
    if (soft) {
        while (PWM_I && PWM_U) {
            if (PWM_I) PWM_I--;
            if (PWM_U) PWM_U--;
            delay_us(10);
        }
        PWM_U = PWM_I = 0;	
        delay_ms(10); // ToDo: remove static delay!
    }
    PWM_ALL_STOP;
    TCCR1A = TCCR1B = 0x00; //stop
    PWM_U = PWM_I = 0;
    ICR1 = TCNT1 = 0;
    delay_ms(10); // ToDo: remove static delay!
    PwmStatus = STOP; //установить признак что PWM не работает
    PWM_set = 0; //Установить признак что значения тока и напряжение не застабилизированы
}

static void Start_PWM_T1 (csu_st mode) {
    if (PwmStatus != STOP) Stop_PWM(HARD);
    ICR1 = MAX_CK; //макс. значение счётчика для режима PWM Frecuency Correct:ICR1;
    PWM_U = PWM_U_NULL; //Задать ширину импульса для канала А
    PWM_I = PWM_I_NULL; //Задать ширину импульса для канала Б
    TCCR1A = 1 << COM1B1 | 1 << WGM11; // OC1A отключен , OC1B инверсный, режим FAST PWM:ICR1.
    if (mode == CHARGE) TCCR1A |= 1 << COM1A1; // OC1A инверсный
    TCCR1B = 1 << WGM12 | 1 << WGM13 | 1 << CS10; //0x11; //CK=CLK ,режим FAST PWM:ICR1
    PwmStatus = mode; //установить признак что PWM работает в режиме заряда
}

void soft_start (uint8_t control_out) {
    Start_PWM_T1(CHARGE); /* запустить преобразователь */
    if (control_out && get_adc_res(ADC_MU) > TaskU) Error = ERR_SET;
    if (!Error) SD(1);
}

uint16_t calc_pwd (uint16_t val, uint8_t limit) {
    int32_t p, k, b;
    k = 1000L * (Clb.setI2 - Clb.setI1) / (Clb.pwm2 - Clb.pwm1);
    b = Clb.pwm1 - (int32_t)Clb.setI1 * 1000L / k;
    p = (int32_t)val * 1000L / k + b;
    if (p < 0) p = 0;
    if (p > MAX_CK) p = MAX_CK;
    if (limit) if ((uint16_t)p > max_pwd_Id) p = max_pwd_Id;
    return (uint16_t)p;	
}

void soft_start_disch (void) {
    Start_PWM_T1(DISCHARGE);
    /* Проверка превышения общей мощности */
    PWM_I = calc_pwd(i_pwr_lim(Cfg.P_maxW, TaskId), 1);
    PWM_U = 0;
    /* установить флаг для калибровки: проконтролировать калибровку */
    Clb.id.bit.control = 1;
}
