#ifndef PWM_H
#define PWM_H
#pragma message	("@(#)pwm.h")

#ifdef	__cplusplus
extern "C" {
#endif

#define PWM_0U          2500
#define PWM_0I          2500
#define MAX_CK          0x1FFF
#define PWM_U           OCR1A
#define PWM_I           OCR1B
#define SOFT            true
#define HARD            false
    
uint16_t PwmDuty (float out, uint16_t null);
void Stop_PWM (bool soft);
void soft_start (uint8_t control_out);
uint16_t calc_pwd (uint16_t val, uint8_t limit);
void soft_start_disch (void);
void Correct_PWM (uint8_t pr);

extern uint16_t max_pwd_I, max_pwd_U, max_pwd_Id;
extern csu_st PwmStatus;
extern uint8_t PWM_set;

#ifdef __cplusplus
}
#endif

#endif // PWM_H