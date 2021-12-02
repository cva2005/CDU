#define REV84 1

#ifdef REV80
#define Rev_ver 8
#define Rev_mode 00
#endif
#ifdef REV81
#define Rev_ver 8
#define Rev_mode 10
#endif
#ifdef REV82
#define Rev_ver 8
#define Rev_mode 20
#endif
#ifdef REV83
#define Rev_ver 8
#define Rev_mode 30
#endif
#ifdef REV84
#define Rev_ver 8
#define Rev_mode 40
#endif

#define Soft_ver 8
#define Soft_mod 49

//---------------------НАСТРОЙКИ ПО УМОЛЧАНИЮ---------------------------
//#define DEBUG_ALG
//#define POWER_24 //конфигурация - источник 24В
//#define CHARGER_24 //конфигурация - зарядник 24В
//-------------------настройка автозапуска-----------
#ifdef POWER_24
	#define AUTOSTART 1
	#define AUTOSTART_CNT 100
	#define AUTOSTART_TIME 100
	#define AUTOSTART_U	U_24V
#else
	#ifdef CHARGER_24
		#define AUTOSTART 1
		#define AUTOSTART_CNT 2
		#define AUTOSTART_TIME 100
		#define AUTOSTART_U	2460
	#else
		#define AUTOSTART 0
		#define AUTOSTART_CNT 100
		#define AUTOSTART_TIME 100
		#define AUTOSTART_U	0
	#endif		
#endif
//-------------------адрес блока---------------------
#define MY_ADR_const 1
//-------------------биты настроек-------------------
#define IN_DATA_bit 0
#define OUT_DATA_bit 0

#define DIAG_WIDE_bit 1
#define I0_SENSE_bit 1
#define LCD_ON_bit 1
#define LED_ON_bit 0
#define PCC_ON_bit 1
#define DEBUG_ON_bit 0
#define GroupM_bit 0
#define EXT_Id_bit 0
#define RELAY_MODE_bit 1
//-------------------кол-во дополнительных РМ-------------------
#define DM_ext 0
//----------------------------------------------------------------------
#define method_def 1 // - количество методов по умолчанию (0 - 1 метод, 1 - 2 метода)

#ifdef REV84 //======================================================ВЕРСИЯ 8.4 (8.49)===============================================================
//-------------------параметры тока и напряжения------
#define maxU_const  4000		//ограничение напряжения		xx00
#define maxI_const  4500		//ограничение тока				xx00
#define maxId_const 1800		//ограничение тока разряда		xx00
#define maxPd_const 2000		//ограничение мощности разряда	xxx0

#define maxId_EXT0	2000		//максимальны16226й ток разряда если нет разрядных модулей
#define maxId_EXT12	5000		//максимальный ток разряда если есть разрядные модули
//-------------------коэффициенты---------------------
#define K_U_const 12730//12615//12650  //R1=26700+1500 R2=1500 Uadc=2,048 Umax=40,55V (K=405500000/(32768-1147)=12375)
#define	K_Up_const 12620//12465//12532  //
#define K_I_const 15516//15692//15589   //R1=10000 R2=383 Uadc=2,048 Ushunt=75.55mV Imax=75.55*50/75=50.37A (K=503666667/32768=15371)
#define K_Id_const 16226//16256//15816  //R1=10000 R2=383 Uadc=2,048 Ushunt=78.45mV Idmax=78.45*50/75=52.3A (K=523000000/32768=15961)
#else //======================================================ДО ВЕРСИИ 8.4==================================================================
//-------------------параметры тока и напряжения------
#define maxU_const  4000		//ограничение напряжения		xx00
#define maxI_const  5000		//ограничение тока				xx00
#define maxId_const 1800		//ограничение тока разряда		xx00
#define maxPd_const 2000		//ограничение мощности разряда	xxx0

#define maxId_EXT0	1800		//максимальный ток разряда если нет разрядных модулей
#define maxId_EXT12	5000		//максимальный ток разряда если есть разрядные модули
//-------------------коэффициенты---------------------
#define K_U_const 12615//12650  //R1=26700+1500 R2=1500 Uadc=2,048 Umax=40,55V (K=405500000/(32768-1147)=12375)
#define	K_Up_const 12465//12532  //
#define K_I_const 15692//15589   //R1=10000 R2=383 Uadc=2,048 Ushunt=75.55mV Imax=75.55*50/75=50.37A (K=503666667/32768=15371)
#define K_Id_const 16226//16256//15816  //R1=10000 R2=383 Uadc=2,048 Ushunt=78.45mV Idmax=78.45*50/75=52.3A (K=523000000/32768=15961)
#endif //=====================================================================================================================================

#define B_U_const 1147    //R1=26700; R2=383; Ushift=0.070709; B_U=Ushift*32767/2.048=1131.31
#define B_Up_const 60	  //Смещение нулевого значения АЦП при измерении U до выходного реле
#define B_I_const 0	  //Смещение нулевого значения АЦП по току заряда
//#define B_I_extId 150//60 //Ток, потребляемый доп. разрядным модулем (дополнитльное смещение 0 при подключении разрядного модуля)
#define B_I_extId_1 0//51
#define B_I_extId_2 0//134
#define B_Id_const 13	  //Смещение нулевого значения АЦП по току разряда

//#define max_PWD_Id_const 5000 //Максимальная величина ШИМ при разряде

//----------коэфициенты для запуска преобразователя--------------------------
//-I разряд-------------------------------
//#define K_PWD_Id 135UL//134UL
//#define B_PWD_Id 0//60
//#define K_PWD_Id_ext1 545 //-1 плата РМ
//#define B_PWD_Id_ext1 18 //-1 плата РМ
//#define K_PWD_Id_ext2 910 //-2 платы РМ
//#define B_PWD_Id_ext2 18 //-2 платы РМ
//-I заряд-------------------------------
#define K_PWD_I 406UL
#define B_PWD_I 52
//-U-------------------------------------
#define K_PWD_U 396//399UL
#define B_PWD_U 27//40

//---------------------------------------------ОШИБКИ-----------------------
#define	ERR_OVERLOAD		1 //перегрузка тока или напряжения при заряде
#define	ERR_DISCH_PWR		2 //перегрузка тока или напряжения при разряде
#define ERR_CONNECTION 		3 //неверное подключение АКБ
#define ERR_NO_AKB			4 //АКБ не подключен
#define ERR_OVERTEMP1		5 //перегрев входного каскада
#define ERR_OVERTEMP2		6 //перегрев выходного каскада
#define ERR_OVERTEMP3		7 //перегрев внешнего радиатора
#define ERR_SET				8 //неверно задано напряжение (задано меньше чем на выходе ЗРМ)
#define ERR_ADC				9 //Неисправность АЦП
#define ERR_STAGE			10 //Неверные параметры этапа
#define ERR_OUT				12 //Короткое замыкание выхода
#define ERR_CONNECTION1		13 //неверное подключение АКБ
#define ERR_DM_LOSS			14 //обрыв разярдного модуля
//----------------------------------------------------------------------
#define MAX_T1 86
#define MAX_T2ch 95
#define MAX_T2dch 95
#define FAN_OFF_T 34
#define FAN_ON_T 37
#define FAN_CND_T 4
//----------------------------------------------------------------------
//#define Good_Con (PINB&0x08)
#define Overload (PIND&0x08)
#define OverTempExt (PINA&0x08)
#define I_St (PINB&0x04)

#define RELAY_ON PORTB=PINB|0x10
#define RELAY_OFF PORTB=PINB&0xEF
#define RELAY_EN (PINB&0x10) //<<0  ВНИМАНИЕ!!! результат всегда должен быть в 5-м бите

#define FAN_ST ((PINB&0x18)>>3)
#define FAN(x) ((x)?(PORTB|=(1<<3)):(PORTB&=~(1<<3)))

#define LED_ERR(x) ((x)?(PORTC|=(1<<7)):(PORTC&=~(1<<7)))
#define ALARM_OUT LED_ERR // управление реле сигнализации
#define ALARM_ON() (PINC7)
#define LED_ERRinv PORTC^=(1<<7);
#define LED_PWR(x) ((x)?(PORTC&=~(1<<6)):(PORTC|=(1<<6)))
#define LED_STI(x) ((x)?(PORTC&=~(1<<5)):(PORTC|=(1<<5)))
#define LED_STU(x) ((x)?(PORTC&=~(1<<4)):(PORTC|=(1<<4)))
#define LED_POL(x) ((x)?(PORTC&=~(1<<3)):(PORTC|=(1<<3)))




#define DE(x) ((x)?(PORTD|=(1<<7)):(PORTD&=~(1<<7)))
#define SD(x) ((x)?(PORTD|=(1<<6)):(PORTD&=~(1<<6)))
#define PWM_ALL_STOP PORTD=PORTD&0x0F //выставить порты pwm SD, DE в 0

//Состояние для переменной PWM_Status (текущий режим ШИМ), ZR_mode (заданный режим), CSU_Enable (Режи в котором запущен блок)
#define stop_charge 0//const for PWM_Status
#define charge 1 //const for PWM_Status
#define discharge 2//const for PWM_Status
#define pulse 3 //const for ZR_mode
#define pause 4 //const for ZR_mode

enum{
	S_STOP,				//0 - преобразователь остановлен
	ST_I_FALL_WAIT,		//1 - ожидание спада тока перед размыканием реле
	S_CHARGE_CAP,		//2- заряд выходных электролитов
	S_VOLTAGE_ALIGNING,	//3 - выравнивание зарядов до и после реле
	S_RELAY_ON,		//4	- замыкание реле
	S_PWM_START,	//5 - запуск предобразователя
	S_WORK,		//6 - работа
};

typedef union
	{
	unsigned int word;
	unsigned char byte[2];
	}ADC_Type;
	
typedef	struct
	{
	unsigned char EEPROM :1;
	unsigned char ADR_SET :1;
	unsigned char IN_DATA :1;
	unsigned char OUT_DATA :1;
	unsigned char TE_DATA :1;
	unsigned char FAN_CONTROL :1;
	unsigned char DIAG_WIDE :1;
	unsigned char I0_SENSE :1;	
	unsigned char LCD_ON :1;
	unsigned char LED_ON :1;
	unsigned char PCC_ON :1;
	unsigned char DEBUG_ON :1;
	unsigned char GroupM :1;
	unsigned char EXT_Id :1;
	unsigned char EXTt_pol :1;
	unsigned char RELAY_MODE :1;
	}CSU_cfg_bit;

typedef union{
	CSU_cfg_bit bit;
	unsigned char byte[2];
	unsigned int word;
} CSU_type;

typedef union{
	struct
		{
		unsigned char autostart :1;
		unsigned char cdu_dsch_dsb :1;
		}bit;
	unsigned char byte[2];
	unsigned int word;
} CSU2_type;

typedef struct 
	{
	signed int	C;
	signed int dC;
	}C_type;

typedef struct  
	{
	//unsigned char en; //автостарт вкл/выкл
	unsigned char restart_cnt; //счётчик перезапусков
	unsigned char cnt_set;
	unsigned char restart_time; //время паузы между перезапусками
	unsigned char time_set;
	unsigned int u_pwm; //минимальное напряженеи рестарта в значениях АЦП
	unsigned int u_set; //минимальное напряженеи рестарта в вольтах*100
	}autosrart_t;

#define ADC_MU 0  //канал АЦП для измерения напряжения
#define ADC_MI 1  //канал АПЦ для измерения тока
#define ADC_DI 2  //канал АЦП для измерения разрядного тока
#define ADC_MUp 3  //канал АЦП для измерения входного тока

//Рабочие значения тока и напряжения (для блокировок)
#define U_0t2V   2000000/K_U
#define U_0t8V   8000000/K_U
#define U_2V  20000000/K_U 
#define U_16V  160000000/K_U 
#define U_20V  200000000/K_U
#define U_24V  240000000/K_U
#define U_24t6V  246000000/K_U
#define U_31V  310000000/K_U

#define I_0t1A  1000000/K_I
#define I_0t2A  2000000/K_I
#define I_1A   10000000/K_I

#define Id_0t1A 1000000/K_Id
#define Id_0t2A 2000000/K_Id
#define Id_1A   10000000/K_Id
#define Id_2A   20000000/K_Id
#define Id_5A   50000000/K_Id
#define Id_8A   80000000/K_Id
#define Id_10A  100000000/K_Id

#ifdef REV84 //======================================================ВЕРСИЯ 8.4===============================================================
//----------коэфициенты для запуска РМ--------------------------
	#define HI_Id_EXT0    59000000/K_Id //граница тока, после которого разрешена калибровка верхнего значения
	#define SETId1_EXT0    Id_0t2A
	#define PWM1_Id_EXT0  60//150 //
	#define SETId2_EXT0  Id_8A
	#define PWM2_Id_EXT0  2391//4665 //

//Для резисторов РМ: верх 0.51к+10к, низ 1к
	#define HI_Id_EXT1	 200000000/K_Id
	#define SETId1_EXT1    3000000/K_Id
	#define PWM1_Id_EXT1 20
	#define SETId2_EXT1  300000000/K_Id
	#define PWM2_Id_EXT1 2375 //1474-30A//1258 - 25А //730 - ток 15А; 473 - 10А

	#define HI_Id_EXT2	 250000000/K_Id
	#define SETId1_EXT2    3000000/K_Id
	#define PWM1_Id_EXT2 11
	#define SETId2_EXT2  300000000/K_Id
	#define PWM2_Id_EXT2 1364//*/
//----------
#else //======================================================ДО ВЕРСИИ 8.4==================================================================
//----------коэфициенты для запуска РМ--------------------------
	#define HI_Id_EXT0    59000000/K_Id
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
//----------
#endif //=====================================================================================================================================

unsigned char U_align_st(void);
unsigned int i_power_limit(unsigned int p, unsigned int i);
void Err_check(void);
void Start_CSU(unsigned char mode);
void Stop_CSU(unsigned char mode);
void Read_temp(void);
void update_LED(void);
void Correct_UI(void);
void Init_Timer0(void);
void Init_ExtInt(void);
void calc_cfg(void);
void Init_Port(void);