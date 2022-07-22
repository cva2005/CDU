#pragma message	("@(#)pwm.c")
#include <system.h>
#include "pwm.h"

csu_st PwmStatus = STOP;

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
    PWM_STOP();
    TCCR1A = TCCR1B = 0x00; //stop
    PWM_U = PWM_I = 0;
    ICR1 = TCNT1 = 0;
    PwmStatus = STOP; //установить признак что PWM не работает
}

void start_pwm (csu_st mode) {
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