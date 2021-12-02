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
//----------
typedef struct
{
	int16_t pwm1;
	int16_t setI1;
	int16_t pwm2;
	int16_t setI2;
	union
	{
		struct
		{
			uint8_t control:1;	//проконтролировать калибровку тока разряда
			uint8_t error:1;	    //ошибка калибровки тока
			uint8_t up_finish:1;	//верхний предел откалиброван
			uint8_t dw_finish:1;	//нижний предел откалиброван
			uint8_t save:1;			//необходимо сохранить значения в EEPROM
		}bit;
		uint8_t byte;
	}id;
	uint8_t crc8;
}calibrate_t;