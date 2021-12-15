#ifndef PWM_H
#define PWM_H
#pragma message	("@(#)pwm.h")

#ifdef	__cplusplus
extern "C" {
#endif

#define P_WDU_start     0
#define P_WDI_start     0
#define MAX_CK          0x1FFF 
#define P_wdI           OCR1B
#define P_wdU           OCR1A

unsigned short PwmDuty (float out);
void Stop_PWM(unsigned char soft);
void soft_start(unsigned char control_out);
unsigned int calculate_pwd(unsigned int val, unsigned char limit);
void soft_start_disch(void);
void Correct_PWM(unsigned char pr);

extern unsigned int max_pwd_I, max_pwd_U, max_pwd_Id;
extern csu_st PWM_status;
extern unsigned char PWM_set;

#ifdef __cplusplus
}
#endif

#endif // PWM_H