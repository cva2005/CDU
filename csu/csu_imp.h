#ifndef CSU_IMP_H
#define CSU_IMP_H
#pragma message	("@(#)csu_imp.h")
#include "tsens/ds1820.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define T_ERR_CNT       8
#define TVAL(x)         (x * 16)
#define MAX_T1          TVAL(86)
#define MAX_T2_CH       TVAL(95)
#define MAX_T2_DCH      TVAL(95)
#define FAN_OFF_T       TVAL(34)
#define FAN_ON_T        TVAL(37)
#define FAN_CND_T       TVAL(4)

#define OVERLD          (PIND & 0x08)
#define OVER_T_EXT      (PINA & 0x08)

#define RELAY_ON()      PORTB |= REL_PIN
#define RELAY_OFF()     PORTB &= ~(REL_PIN)

#define ALARM_ON()      (PINC7)
#define LED_ERRinv      PORTC ^= 1 << 7;
#define LED_PWR(x)      x ? (PORTC &= ~(1 << 6)) : (PORTC |= 1 << 6)
#define LED_STI(x)      x ? (PORTC &= ~(1 << 5)) : (PORTC |= 1 << 5)
#define LED_STU(x)      x ? (PORTC &= ~(1 << 4)) : (PORTC |= 1 << 4)
#define LED_POL(x)      x ? (PORTC &= ~(1 << 3)) : (PORTC |= 1 << 3)
#define DISCH_EN(x)     x ? (PORTD |= 1 << 7) : (PORTD &= ~(1 << 7))
#define CHARGE_EN(x)    x ? (PORTD |= 1 << 6) : (PORTD &= ~(1 << 6))

#define ADC_MU          0 // канал измерения напряжения
#define ADC_MI          1 // канал измерения тока
#define ADC_DI          2 // канал измерения разрядного тока
#define ADC_MUp         3 // канал измерения входного тока

//----------коэфициенты для запуска РМ--------------------------
#define HI_Id_EXT0      59000000 / Cfg.K_Id //граница тока, после которого разрешена калибровка верхнего значения
#define SETId1_EXT0     ID_A(0,2)
#define PWM1_Id_EXT0    60//150 //
#define SETId2_EXT0     ID_A(8,0)
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

#define OUT_ERR_MAX     3
#define EXT_ERR_VAL     100
#define INF_TAU         5.0f
#define STABLE_I        2500
#define DIFF_U          500
#define K_FIN           0.03f
#define LIM_U           300
#define STBL_N          70
#define TD_STABLE       20.0f
#define ADC_BANDW       3000.0 // FLT_MAX
#define CURR_REF        12888.0f
#define TMP_PRD         MS(1830)
#define PWR_TIME        MS(160) // led power time
#define CNTRL_T         MS(100) // control dicrete time
#define BREAK_T         SEC(30) // ToDo: вернуть на 5-7 сек!
#define U_MAX_VAL       4000
#define P_MAX_EXT0      2500
#define P_MAX_EXT1      8000
#define P_MAX_EXT2      14000

#ifdef __cplusplus
}
#endif

#endif /* CSU_IMP_H */