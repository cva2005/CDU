#pragma message	("@(#)pwm.c")
#include <system.h>
#include "csu/csu.h"
#include "lcd/wh2004.h"
#include "spi/adc/ads1118.h"
#include "pwm.h"

csu_st PwmStatus = STOP;

static void start_pwm (csu_st mode);

csu_st pwm_state (void) {
    return PwmStatus;
}

uint16_t pwm_duty (float out, uint16_t null) {
    if (out >= 1.0f) return MAX_CK;
    if (out <= 0) return null;
    out *= MAX_CK - null;
    return (uint16_t)out + null;
}

void stop_pwm (bool soft) {
    if (soft) {
        while (PWM_I && PWM_U) {
            if (PWM_I) PWM_I--;
            if (PWM_U) PWM_U--;
            delay_us(2);
        }
        PWM_U = PWM_I = 0;	
    }
    PWM_ALL_STOP;
    TCCR1A = TCCR1B = 0x00; //stop
    PWM_U = PWM_I = 0;
    ICR1 = TCNT1 = 0;
    PwmStatus = STOP; //установить признак что PWM не работает
}

static void start_pwm (csu_st mode) {
    if (PwmStatus != STOP) stop_pwm(HARD);
    ICR1 = MAX_CK; //макс. значение счётчика для режима PWM Frecuency Correct:ICR1;
    TCCR1A = 1 << COM1B1 | 1 << WGM11; // OC1A отключен , OC1B инверсный, режим FAST PWM:ICR1.
    if (mode == CHARGE) {
        TCCR1A |= 1 << COM1A1; // OC1A инверсный
        PWM_U = PWM_0U; //Задать ширину импульса для канала А
        PWM_I = MAX_CK; //Задать ширину импульса для канала Б
    } else {
        /* ToDo: Проверка превышения общей мощности */
        PWM_U = 0;
        PWM_I = PWM_0I;
    }
    TCCR1B = 1 << WGM12 | 1 << WGM13 | 1 << CS10; //0x11; //CK=CLK ,режим FAST PWM:ICR1
    PwmStatus = mode; //установить признак что PWM работает
}

void soft_start (uint8_t control_out) {
    start_pwm(CHARGE); /* запустить преобразователь */
    if (control_out && get_adc_res(ADC_MU) > TaskU) Error = ERR_SET;
    if (!Error) SD(1);
}

void soft_start_disch (void) {
    start_pwm(DISCHARGE);
    /* установить флаг для калибровки: проконтролировать калибровку */
    Clb.id.bit.control = 1;
}
