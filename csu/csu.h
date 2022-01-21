#ifndef CSU_H
#define CSU_H
#pragma message	("@(#)csu.h")
#include "tsens/ds1820.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define HW_VER          8
#define HW_MODE         40
#define SW_VER          9
#define SW_MODE         00
#define SOFTW_VER       "v.9.00"
#define HARDW_VER       "v.8.40"

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
typedef enum {
    NO_ERR            = 0, //перегрузка тока или напряжения при заряде
    ERR_OVERLOAD      = 1, //перегрузка тока или напряжения при заряде
    ERR_DISCH_PWR     = 2, //перегрузка тока или напряжения при разряде
    ERR_CONNECTION    = 3, //неверное подключение АКБ
    ERR_NO_AKB        = 4, //АКБ не подключен
    ERR_OVERTEMP1     = 5, //перегрев входного каскада
    ERR_OVERTEMP2     = 6, //перегрев выходного каскада
    ERR_OVERTEMP3     = 7, //перегрев внешнего радиатора
    ERR_SET           = 8, //неверно задано напряжение (задано меньше чем на выходе ЗРМ)
    ERR_ADC           = 9, //Неисправность АЦП
    ERR_STG           = 10, //Неверные параметры этапа
    ERR_OUT           = 12, //Короткое замыкание выхода
    ERR_CONNECTION1	  = 13, //неверное подключение АКБ
    ERR_DM_LOSS		  = 14 //обрыв разярдного модуля
} err_t;

//----------------------------------------------------------------------
#define T_ERR_CNT           8
#define TVAL(x)             (x * 16)
#define MAX_T1              TVAL(86)
#define MAX_T2ch            TVAL(95)
#define MAX_T2dch           TVAL(95)
#define FAN_OFF_T           TVAL(34)
#define FAN_ON_T            TVAL(37)
#define FAN_CND_T           TVAL(4)
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

#define LED_ERR(x) x ? (PORTC |= 1 << 7) : (PORTC &= ~(1 << 7))
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

//Состояние для переменной PwmStatus (текущий режим ШИМ), SetMode (заданный режим), CsuState (Режи в котором запущен блок)
typedef enum {
    STOP        = 0, //const for PwmStatus
    CHARGE      = 1, //const for PwmStatus
    DISCHARGE   = 2, //const for PwmStatus
    PULSE       = 3, //const for SetMode
    PAUSE       = 4  //const for SetMode
} csu_st;

enum{
	S_STOP,				//0 - преобразователь остановлен
	ST_I_FALL_WAIT,		//1 - ожидание спада тока перед размыканием реле
	S_CHARGE_CAP,		//2- заряд выходных электролитов
	S_VOLTAGE_ALIGNING,	//3 - выравнивание зарядов до и после реле
	S_RELAY_ON,		//4	- замыкание реле
	S_PWM_START,	//5 - запуск предобразователя
	S_WORK,		//6 - работа
};

typedef union {
	unsigned int word;
	unsigned char byte[2];
} ADC_Type;
	
typedef struct {
    unsigned eepr       :1;
    unsigned addr_set   :1;
    unsigned in_data    :1;
    unsigned out_data   :1;
    unsigned te_data    :1;
    unsigned fan_cntrl  :1;
    unsigned diag_wide  :1;
    unsigned io_sense   :1;
} cmd_t;

typedef struct {
    unsigned lcd        :1;
    unsigned led        :1;
    unsigned pcc        :1;
    unsigned dbg        :1;
    unsigned group      :1;
    unsigned ext_id     :1;
    unsigned ext_pol    :1;
    unsigned reley      :1;
} mode_t;

typedef	struct {
    unsigned astart     :1;
    unsigned dsch_dsb   :1;
} bf2_t;

typedef struct  {
    signed int	C;
    signed int dC;
} cap_t;

typedef struct {
    unsigned char err_cnt; //счётчик перезапусков
    stime_t rst_time; //время паузы между перезапусками
    unsigned int u_pwm; //минимальное напряженеи рестарта в значениях АЦП
    unsigned int u_set; //минимальное напряженеи рестарта в вольтах*100
} ast_t;

typedef enum {
    RESET_ST,
    CHARGE_ST,
    DISCHARGE_ST
} state_t;

#define ADC_MU      0 // канал измерения напряжения
#define ADC_MI      1 // канал измерения тока
#define ADC_DI      2 // канал измерения разрядного тока
#define ADC_MUp     3 // канал измерения входного тока

//Рабочие значения тока и напряжения (для блокировок)
#define U_V(x,y)    (uint32_t)(x * 10000000UL + y * 1000000UL) / Cfg.K_U
#define I_A(x,y)    (uint32_t)(x * 10000000UL + y * 1000000UL) / Cfg.K_I
#define Id_A(x,y)   (uint32_t)(x * 10000000UL + y * 1000000UL) / Cfg.K_Id
#define U_adc(x)    (uint32_t)(x * 100000UL) / Cfg.K_U
#define I_adc(x)    (uint32_t)(x * 100000UL) / Cfg.K_I
#define Id_adc(x)   (uint32_t)(x * 100000UL) / Cfg.K_Id
#define U_m(x,y)    (uint32_t)x * (uint32_t)(y * 10) / Cfg.K_U
#define I_m(x,y)    (uint32_t)x * (uint32_t)(y * 10) / Cfg.K_I
#define Id_m(x,y)   (uint32_t)x * (uint32_t)(y * 10) / Cfg.K_Id

//----------коэфициенты для запуска РМ--------------------------
#define HI_Id_EXT0      59000000 / Cfg.K_Id //граница тока, после которого разрешена калибровка верхнего значения
#define SETId1_EXT0     Id_A(0,2)
#define PWM1_Id_EXT0    60//150 //
#define SETId2_EXT0     Id_A(8,0)
#define PWM2_Id_EXT0    2391//4665 //
//Для резисторов РМ: верх 0.51к+10к, низ 1к
#define HI_Id_EXT1	    200000000 / Cfg.K_Id
#define SETId1_EXT1     3000000 / Cfg.K_Id
#define PWM1_Id_EXT1    20
#define SETId2_EXT1     300000000 / Cfg.K_Id
#define PWM2_Id_EXT1    2375 //1474-30A//1258 - 25А //730 - ток 15А; 473 - 10А
#define HI_Id_EXT2	    250000000 / Cfg.K_Id
#define SETId1_EXT2     3000000 / Cfg.K_Id
#define PWM1_Id_EXT2    11
#define SETId2_EXT2     300000000 / Cfg.K_Id
#define PWM2_Id_EXT2    1364//*/

#define EXT_ERR_VAL     100
#define INF_TAU         50.0f
#define PWR_TIME        MS(160) // led power time
#define CNTRL_T         MS(300) // control dicrete time
//#define DOWN_LIM    100.0f

unsigned char U_align_st (void);
uint16_t i_pwr_lim (uint16_t p, uint16_t i);
void Init_ExtInt (void);
void csu_start (csu_st mode);
void csu_stop (csu_st mode);
void csu_drv (void);
void calc_cfg (void);

extern uint16_t TaskI, TaskU, TaskId;
extern uint16_t MaxI, MaxId, MaxU;
extern err_t Error;
extern bool SelfCtrl, pLim;
extern uint16_t id_dw_Clb, id_up_Clb;
extern csu_st CsuState, SetMode;
extern stime_t AlarmDel;
extern uint16_t ADC_O[];
extern int16_t Tmp[T_N];

#ifdef __cplusplus
}
#endif

#endif /* CSU_H */