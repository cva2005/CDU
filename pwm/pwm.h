#pragma message	("@(#)pwm.h")
#define P_WDU_start 0
#define P_WDI_start 0
#define MAX_CK 0x1FFF 
#define P_wdI OCR1B
#define P_wdU OCR1A

unsigned short PwmDuty (float out);
void Stop_PWM(unsigned char soft);
void Start_PWM_T1(unsigned char mode);
//void Stop_PWM_T1(void);
void soft_start(unsigned char control_out);
//unsigned char step_up (unsigned char step);
//unsigned char step_down (unsigned char step);

//void stabilization(unsigned int U, unsigned int I);

//void Start_PWM_T2(void);
//void Stop_PWM_T2(void);
unsigned int calculate_pwd(unsigned int val, unsigned char limit);
void soft_start_disch(void);
void Correct_PWM(unsigned char pr);
#if 0
//----------коэфициенты для запуска РМ--------------------------
#define HI_Id_EXT0    60000000/K_Id
#define SETId1_EXT0    2000000/K_Id //0,2A
#define PWM1_Id_EXT0  150 //
#define SETId2_EXT0  100000000/K_Id 
#define PWM2_Id_EXT0  4665 //

//Для резисторов РМ: верх 0.51к+10к, низ 1к
#define HI_Id_EXT1	 200000000/K_Id
#define SETId1_EXT1    3000000/K_Id
#define PWM1_Id_EXT1 51
#define SETId2_EXT1  300000000/K_Id
#define PWM2_Id_EXT1 4123 //1474-30A//1258 - 25А //730 - ток 15А; 473 - 10А

#define HI_Id_EXT2	 250000000/K_Id
#define SETId1_EXT2    3000000/K_Id
#define PWM1_Id_EXT2 17
#define SETId2_EXT2  300000000/K_Id
#define PWM2_Id_EXT2 2326//*/
#endif
