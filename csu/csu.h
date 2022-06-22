#ifndef CSU_H
#define CSU_H
#pragma message	("@(#)csu.h")
#include "tsens/ds1820.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    NO_ERR            = 0,  // перегрузка тока или напряжения при заряде
    ERR_OVERLOAD      = 1,  // перегрузка тока или напряжения при заряде
    ERR_DISCH_PWR     = 2,  // перегрузка тока или напряжения при разряде
    ERR_CONNECTION    = 3,  // неверное подключение АКБ
    ERR_NO_AKB        = 4,  // АКБ не подключен
    ERR_OVERTEMP1     = 5,  // перегрев входного каскада
    ERR_OVERTEMP2     = 6,  // перегрев выходного каскада
    ERR_OVERTEMP3     = 7,  // перегрев внешнего радиатора
    ERR_SET           = 8,  // неверно задано напряжение (задано меньше чем на выходе ЗРМ)
    ERR_ADC           = 9,  // Неисправность АЦП
    ERR_STG           = 10, // Неверные параметры этапа
    ERR_OUT           = 12, // Короткое замыкание выхода
    ERR_CONNECTION1	  = 13, // неверное подключение АКБ
    ERR_DM_LOSS		  = 14  // обрыв разярдного модуля
} err_t;

#define I_ST            (PINB & 0x04)
#define FAN(x)          x ? (PORTB |= 1 << 3) : (PORTB &= ~(1 << 3))
#define FAN_ST          ((PINB & 0x18) >> 3)
#define REL_PIN         0x10
#define IS_RELAY_EN()  (PORTB & REL_PIN)
#define ALARM_OUT       LED_ERR // управление реле сигнализации
#define LED_ERR(x)      x ? (PORTC |= 1 << 7) : (PORTC &= ~(1 << 7))
#define PWM_STOP()      PORTD &= 0x0F //выставить порты pwm SD, DE в 0

/* Состояние для переменной PwmStatus (текущий режим ШИМ),
SetMode (заданный режим), CsuState (Режи в котором запущен блок) */
typedef enum {
    STOP        = 0, //const for PwmStatus
    CHARGE      = 1, //const for PwmStatus
    DISCHARGE   = 2, //const for PwmStatus
    PULSE       = 3, //const for SetMode
    PAUSE       = 4  //const for SetMode
} csu_st;

typedef enum {
    TCH0,
    TCH1
} tch_t;

enum{
	S_STOP,             //0 - преобразователь остановлен
	ST_I_FALL_WAIT,     //1 - ожидание спада тока перед размыканием реле
	S_CHARGE_CAP,       //2- заряд выходных электролитов
	S_VOLTAGE_ALIGNING, //3 - выравнивание зарядов до и после реле
	S_RELAY_ON,         //4	- замыкание реле
	S_PWM_START,        //5 - запуск предобразователя
	S_WORK,             //6 - работа
};

typedef union {
	unsigned int word;
	unsigned char byte[2];
} ADC_Type;

typedef struct {
    uint8_t eepr       :1;
    uint8_t addr_set   :1;
    uint8_t in_data    :1;
    uint8_t out_data   :1;
    uint8_t te_data    :1;
    uint8_t fan_cntrl  :1;
    uint8_t diag_wide  :1;
    uint8_t io_sense   :1;
} cmd_t;

typedef struct {
    uint8_t lcd        :1;
    uint8_t led        :1;
    uint8_t pcc        :1;
    uint8_t dbg        :1;
    uint8_t group      :1;
    uint8_t ext_id     :1;
    uint8_t ext_pol    :1;
    uint8_t reley      :1;
} mode_t;

typedef union {
    struct {
        uint8_t astart      :1;
        uint8_t dsch_dsb    :1;
        uint8_t pulse       :1;
    } bit;
	uint16_t word;
} bf2_t;

typedef struct {
    int16_t	C;
    int16_t dC;
} cap_t;

typedef struct {
    uint8_t err_cnt; //счётчик перезапусков
    stime_t rst_time; //время паузы между перезапусками
    uint16_t u_pwm; //минимальное напряженеи рестарта в значениях АЦП
    uint16_t u_set; //минимальное напряженеи рестарта в вольтах*100
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
#define ID_A(x,y)   (uint32_t)(x * 10000000UL + y * 1000000UL) / Cfg.K_Id
#define U_ADC(x)    (uint32_t)(x * 100000UL) / Cfg.K_U
#define I_ADC(x)    (uint32_t)(x * 100000UL) / Cfg.K_I
#define ID_ADC(x)   (uint32_t)(x * 100000UL) / Cfg.K_Id
#define U_M(x,y)    (uint32_t)x * (uint32_t)(y * 10) / Cfg.K_U
#define I_M(x,y)    (uint32_t)x * (uint32_t)(y * 10) / Cfg.K_I
#define ID_M(x,y)   (uint32_t)x * (uint32_t)(y * 10) / Cfg.K_Id

unsigned char U_align_st (void);
void Init_ExtInt (void);
void csu_start (csu_st mode);
void csu_stop (csu_st mode);
void csu_drv (void);
void calc_cfg (void);
err_t get_csu_err (void);
void set_csu_err (err_t err);
void set_task_ic (uint16_t task);
void set_task_id (uint16_t task);
void set_task_u (uint16_t task);
uint16_t get_task_ic (void);
uint16_t get_task_id (void);
uint16_t get_task_u (void);
int16_t get_csu_t (tch_t ch);
extern uint16_t MaxI, MaxId, MaxU;
extern bool SelfCtrl, pLim;
extern uint16_t id_dw_Clb, id_up_Clb;
extern csu_st CsuState, SetMode;
extern stime_t AlarmDel;
extern uint16_t ADC_O[];

#ifdef __cplusplus
}
#endif

#endif /* CSU_H */