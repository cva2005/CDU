#pragma message	("@(#)csu.c")
#include <system.h>
#include "lcd/lcd.h"
#include "pwm/pwm.h"
#include "spi/adc/ads1118.h"
#include "csu/mtd.h"
#include "pid/pid_r.h"
#include "key/key.h"
#include "csu/csu.h"

static inline void check_auto_start (void);
static inline void csu_control (void);
static inline void start_cntrl (void);
static inline void update_led (void);
static inline err_t err_check (void);
static inline void read_tmp (void);
static void out_off (void);

static stime_t LcdRefr, AlarmDel, FanTime;
static uint8_t ErrT[T_N] = {0, 0}; /* error count for T sensors */
static stime_t BreakTime, TickSec, LedPwrTime, CntrlTime;
ast_t AutoStr;
int16_t Tmp[T_N];
uint16_t id_dw_Clb, id_up_Clb;
uint16_t ADC_O[ADC_CH]; //данные АЦП без изменений (бе вычета коэфициента В)
static float Uerr, Ierr, InfTau;
static uint16_t  dm_loss_cnt = 0;
static uint8_t ERR_Ext = 0, OUT_err_cnt = 0;
static bool InitF, SatU, InitCsu = false;
bool pLim = false, LedPwr;
bool SelfCtrl = false; //упр-е мет. заряда самостоятельно или удалённо
err_t Error = NO_ERR;
csu_st CsuState, SetMode = STOP;
#define K_P         0.0000055f  /* Kp; gain factor */
#define T_I         5.0f        /* Ti integration time */
#define T_F         5.0f        /* Tf derivative filter tau */
#define T_D         0.2f        /* Td derivative time */
#define PLS_K_P     0.0002f
#define PLS_T_I     10.0f
#define PLS_T_F     5.0f
#define PLS_T_D     0.01f
static pid_t Pid_U;
static const pid_t UPidDef = {
    K_P,
    T_I,
    T_F,
    T_D,
	/* i[ST_SIZE] old input states */
#if ST_SIZE == 2
    0.0, 0.0,
#else /* ST_SIZE == 4 */
    0.0, 0.0, 0.0, 0.0,
#endif
	0.0,    /* u old output state */
	0.0,    /* d old derivative state */
	0.0,    /* Xd dead zone */
	0.0     /* Xi integral zone */
};
static const pid_t UPidPlsDef = {
    PLS_K_P,
    PLS_T_I,
    PLS_T_F,
    PLS_T_D,
	/* i[ST_SIZE] old input states */
#if ST_SIZE == 2
    0.0, 0.0,
#else /* ST_SIZE == 4 */
    0.0, 0.0, 0.0, 0.0,
#endif
	0.0,    /* u old output state */
	0.0,    /* d old derivative state */
	0.0,    /* Xd dead zone */
	0.0     /* Xi integral zone */
};
static pid_t Pid_Ic;
static const pid_t IcPidDef = {
    K_P,
    T_I,
    T_F,
    T_D,
    /* i[ST_SIZE] old input states */
#if ST_SIZE == 2
    0.0, 0.0,
#else /* ST_SIZE == 4 */
    0.0, 0.0, 0.0, 0.0,
#endif
    0.0,    /* u old output state */
    0.0,    /* d old derivative state */
    0.0,    /* Xd dead zone */
    0.0     /* Xi integral zone */
};
static const pid_t IcPidPlsDef = {
    PLS_K_P,
    PLS_T_I,
    PLS_T_F,
    PLS_T_D,
    /* i[ST_SIZE] old input states */
#if ST_SIZE == 2
    0.0, 0.0,
#else /* ST_SIZE == 4 */
    0.0, 0.0, 0.0, 0.0,
#endif
    0.0,    /* u old output state */
    0.0,    /* d old derivative state */
    0.0,    /* Xd dead zone */
    0.0     /* Xi integral zone */
};
#define DK_P        0.00005f /* Kp; gain factor */
#define DT_I        3.0f     /* Ti integration time */
#define DT_F        5.0f     /* Tf derivative filter tau */
#define DT_D        0.001f   /* Td derivative time */
#define PLS_DK_P    0.00001f
#define PLS_DT_I    3.0f
#define PLS_DT_F    5.0f
#define PLS_DT_D    0.001f
static pid_t Pid_Id;
static const pid_t IdPidDef = {
    DK_P, /* Kp; gain factor */
    DT_I, /* Ti integration time */
    DT_F, /* Tf derivative filter tau */
    DT_D, /* Td derivative time */
    /* i[ST_SIZE] old input states */
#if ST_SIZE == 2
    0.0, 0.0,
#else /* ST_SIZE == 4 */
    0.0, 0.0, 0.0, 0.0,
#endif
    0.0,    /* u old output state */
    0.0,    /* d old derivative state */
    0.0,    /* Xd dead zone */
    0.0     /* Xi integral zone */
};

static const pid_t IdPidPlsDef = {
    PLS_DK_P, /* Kp; gain factor */
    PLS_DT_I, /* Ti integration time */
    PLS_DT_F, /* Tf derivative filter tau */
    PLS_DT_D, /* Td derivative time */
    /* i[ST_SIZE] old input states */
#if ST_SIZE == 2
    0.0, 0.0,
#else /* ST_SIZE == 4 */
    0.0, 0.0, 0.0, 0.0,
#endif
    0.0,    /* u old output state */
    0.0,    /* d old derivative state */
    0.0,    /* Xd dead zone */
    0.0     /* Xi integral zone */
};

void csu_drv (void) {
    if (!InitCsu) {
        FanTime = get_fin_time(TMP_PRD);
        InitCsu = true;
    }
    if (pwm_state() != STOP && !get_time_left(TickSec)) {
        TickSec = get_fin_time(SEC(1));
        lcd_tick_sec();
        Tm.s++; // секунды метода
        Ts.s++; // секунды этапа
        if (Tm.s > 59) {
            Tm.m++; // минуты метода
            Tm.s = 0;
        }
        if (Tm.m > 59) {
            if (Tm.h < 255) Tm.h++;	//Часы метода
            Tm.m = 0;
        }
        if (Ts.s > 59) {
            Ts.m++; // Минты этапа
            Ts.s = 0;
        }
        if (Ts.m > 59) {
            Ts.h++;	//Часы этапа
            Ts.m = 0;
        }
    }
    /* Индикация ошибок для LED */
    if (Cfg.mode.led && CsuState == STOP
        && !get_time_left(LedPwrTime)) {
        LedPwrTime = get_fin_time(PWR_TIME);
        LED_PWR(LedPwr);
        LedPwr = !LedPwr;
    }
    /* FAN control section */
#ifdef T_LONG_READ
    if (!get_time_left(FanTime) && (!rs_busy() || !rs_active())) {
        rs_set_busy();
#else
    if (tmp_drv() == COMPL && !get_time_left(FanTime)) {
#endif
        FanTime = get_fin_time(TMP_PRD);
        read_tmp();
        if (!Cfg.cmd.fan_cntrl) {
            int16_t t1 = Tmp[0]; int16_t t2 = Tmp[1]; 
            if (t1 > FAN_ON_T || t2 > FAN_ON_T ||
                (t1 >= -FAN_CND_T && t1 < FAN_CND_T) ||
                (t2 >= -FAN_CND_T && t2 < FAN_CND_T) ||
                t1 == ERR_WCODE || t2 == ERR_WCODE) FAN(ON);
            else if (t1 < FAN_OFF_T && t2 < FAN_OFF_T) FAN(OFF);
        }
    }
    if (Error = err_check()) {
        /* если преобразователь не выключен: выключить преобразователь */
        if (pwm_state() != STOP) stop_pwm(HARD);
        if (IS_RELAY_EN()) out_off();
    }
    if (!Cfg.mode.led || rs_active()) csu_control();
    if (Clb.id.bit.save == 1) {
        if (!Clb.id.bit.error
            && (Clb.id.bit.dw_Fin || Clb.id.bit.up_Fin)) {
            save_clb();
            Clb.id.byte = 0;
        }
    }
    if (Cfg.mode.lcd) {
        if (!rs_active()) { //если подключение прервалось, а значения дисплея не обновлены
            if (lcd_conn_msg()) {
                if (Cfg.cmd.diag_wide) {
                    csu_stop(STOP);
                    read_mtd();
                }
                lcd_wr_set();
            }				
        } else	{ //если подключение появилось, а значения дисплея не обновлены
            if (!Cfg.mode.dbg && !lcd_conn_msg()) lcd_wr_connect(true);
        }			
    }
    if (!rs_active() || Cfg.mode.dbg) {
        if (Cfg.mode.led) update_led();
        if (Cfg.mode.lcd) {
            if (!get_time_left(LcdRefr)) {
                lcd_update_work();
                if (CsuState == STOP)
                LcdRefr = get_fin_time(MS(400));
            }
        }
        if (CsuState && SelfCtrl) {
            /* Если идёт заряд или разряд и управление зарядом
             * осуществляет ЗРМ самостоятельно: проверить
             * статус этапа (испульсный режим или нет),
             * в случае необходимости изменить исмпульс */
            stg_status();
        }
        check_key();
        check_auto_start();
    }
    if (ALARM_ON()) {
        if (!get_time_left(AlarmDel)) {
            ALARM_OUT(OFF);
        }
    }
}

void calc_cfg (void) {
    Error = NO_ERR;
    if (Cfg.maxI > 5000) Cfg.maxI = 5000;
    MaxI = (uint32_t)Cfg.maxI * 100000UL / Cfg.K_I;
    if (Cfg.mode.group) MaxI = MaxI + I_A(1,0);
    if (Cfg.dmSlave == 0) {
        if (Cfg.maxId > MAX_ID_EXT0) Cfg.maxId = MAX_ID_EXT0;
    } else {
        if (Cfg.maxId > MAX_ID_EXT1) Cfg.maxId = MAX_ID_EXT1;
    }
    MaxId = (uint32_t)Cfg.maxId * 100000UL / Cfg.K_Id;
    if (Cfg.mode.group) MaxId = MaxId + I_A(1,0);
    if (Cfg.maxU > 4000) Cfg.maxU = 4000;
    MaxU = (uint32_t)Cfg.maxU * 100000UL / Cfg.K_U;
    if (Cfg.dmSlave > 2) Cfg.dmSlave = 2;
    if (Cfg.dmSlave > 0) Cfg.mode.ext_id = 1;
    else Cfg.mode.ext_id = 0;
    ADC_O[ADC_MU] = Cfg.B[ADC_MU];
    ADC_O[ADC_MI] = Cfg.B[ADC_MI];
    AutoStr.u_pwm = ((uint32_t)AutoStr.u_set * 100000UL) / Cfg.K_U;
    AutoStr.err_cnt = Cfg.cnt_set;
    if (Cfg.bf2.bit.astart) Cfg.mode.dbg = 1;
    id_dw_Clb = ID_A(2,0);
    if (Cfg.dmSlave == 0) {
        id_up_Clb = HI_Id_EXT0;
        if (Cfg.P_maxW > 2500) Cfg.P_maxW = 2500;
	}
    if (Cfg.dmSlave == 1) {
        id_up_Clb = HI_Id_EXT1;
        if (Cfg.P_maxW > 8000) Cfg.P_maxW = 8000;
	}
    if (Cfg.dmSlave > 1) {
        id_up_Clb = HI_Id_EXT2;
        if (Cfg.P_maxW > 14000) Cfg.P_maxW = 14000;
	}
    if (id_up_Clb > MaxId) id_up_Clb = MaxId >> 1;
    if (read_clb() == false) {
        if (Cfg.dmSlave == 0) {
            Clb.pwm1 = PWM1_Id_EXT0;
            Clb.setI1 = SETId1_EXT0;
            Clb.pwm2 = PWM2_Id_EXT0;
            Clb.setI2 = SETId2_EXT0;
        }
        if (Cfg.dmSlave == 1) {
            Clb.pwm1 = PWM1_Id_EXT1;
            Clb.setI1 = SETId1_EXT1;
            Clb.pwm2 = PWM2_Id_EXT1;
            Clb.setI2 = SETId2_EXT1;
        }
        if (Cfg.dmSlave > 1) {
            Clb.pwm1 = PWM1_Id_EXT2;
            Clb.setI1 = SETId1_EXT2;
            Clb.pwm2 = PWM2_Id_EXT2;
            Clb.setI2 = SETId2_EXT2;
        }
        Clb.id.byte = 0;
	}
    lcd_clear();
    if (Cfg.mode.lcd) Init_WH2004(1);
    else Init_WH2004(0);	
    if (Cfg.mode.led) {
        DDRC = 0xFF;
        PORTC = 0xFF;	
        LedPwrTime = get_fin_time(PWR_TIME);
	}
}

void csu_start (csu_st mode) {
    if (Error = err_check()) return;
    bool task_u = false;
    if (CsuState == STOP) {
        start_cntrl();
        if (TaskU && Cfg.cmd.diag_wide
            && !!Cfg.mode.group) task_u = true;
    }
    if (pwm_state() != STOP) {
        stop_pwm(SOFT);
        pLim = false;
    }
    CsuState = mode;
    if (mode == PAUSE) {
        if (IS_RELAY_EN()) out_off();
        return;
    }
    RELAY_ON();
    if (mode == CHARGE) soft_start(task_u);
    else if (mode == DISCHARGE) {
        DE(1);
        soft_start_disch();
    }
    BreakTime = get_fin_time(SEC(1));
}

void csu_stop (csu_st mode) {
    if (!Error) AutoStr.err_cnt = Cfg.cnt_set;
    Error = NO_ERR;
    stop_pwm(SOFT);
    InitF = pLim = false;
    CsuState = STOP;
    //delay_ms(10);
    if (mode == STOP) out_off();
    else RELAY_ON();
}

uint16_t i_pwr_lim (uint16_t p, uint16_t i) {
    uint32_t u, id;
    pLim = false;
    u = get_adc_res(ADC_MU);
    u *= Cfg.K_U;
    u /= D1M;
    if (u > 0) {
        id = (uint64_t)(p * D100M) / u / Cfg.K_Id;
        if (id < 32768 && id > 0) {
            if (i > (uint16_t)id) {
                /* если превышена мощность, то снизить ток
                 * и установить флаг ограничения мощности */
                i = id; 
                pLim = true;
            }
        }
    }
    return i;
}

static inline err_t err_check (void) {
    csu_st pwm_st = pwm_state();
    /* Проверка ошибки по внешнему входу */
    if (!Cfg.mode.led && Cfg.mode.ext_id) {
    /* индикация не светодиодная и включено
     * управление вншним разрядным модулем */
        if (OVER_T_EXT != Cfg.mode.ext_pol) ERR_Ext++;
        /* проверка срабатывания внешнего термореле */
        else ERR_Ext = 0;
        if (ERR_Ext > EXT_ERR_VAL) return ERR_OVERTEMP3;
    }
    /* Проверка на обрыв РМ */
	if (Cfg.dmSlave) {
		if (Cfg.cmd.diag_wide && pwm_st == DISCHARGE) {
        /* включена расширеная диагностика */
            if (dm_loss_cnt > 0 && dm_loss_cnt < 10) {
                if (get_adc_res(ADC_DI) > ID_A(0,1)) {
                    if (TaskU < get_adc_res(ADC_MU) && !pLim
                        && TaskId > get_adc_res(ADC_DI) << 1) {
                        return ERR_DM_LOSS;
                    }
                    if (TaskId > get_adc_res(ADC_DI) + ID_A(0,2)) {
                        /* реальный ток в 2 раза меньше заданного */
                        return ERR_DM_LOSS;
                    }
                }
            }
        }
    }
    /* Проверка на обрыв нагрузки */
    if (Cfg.cmd.io_sense && !Cfg.mode.group) {
         /* диагностика обрыва нагрузки и блок не в группе */
        if (pwm_st == CHARGE && TaskI) {
            if (get_adc_res(ADC_MI) >= I_A(0,1)) goto set_break_time;
            if (!get_time_left(BreakTime) && PWM_I > 0) return ERR_NO_AKB;
        }
        if (pwm_st == DISCHARGE && TaskId) {
            if (get_adc_res(ADC_DI) >= ID_A(0,1)) {
            set_break_time:
                BreakTime = get_fin_time(SEC(1));
            }
            if (!get_time_left(BreakTime) && (PWM_I > 0)) return ERR_NO_AKB;
        }
    }
    /* Проверка перегрева */
    if (pwm_st == DISCHARGE) {
        goto check_over_t2;
    } else if (pwm_st == CHARGE) {
        if ((Tmp[0] > MAX_T1) && (Tmp[0] != ERR_WCODE))
            /* проверка перегрева транзисторов */
            return ERR_OVERTEMP1;
        check_over_t2:
        if ((Tmp[1] > MAX_T2_CH) && (Tmp[1] != ERR_WCODE))
            /* проверка перегрева выпрямительных диодов */
        return ERR_OVERTEMP2;
    }
    /* Проверка полярности подключения АКБ (переполюсовка) */
    if (!Cfg.mode.group) {
        int16_t sub = Cfg.B[ADC_MU] - ADC_O[ADC_MU];
        if (sub > 300) return ERR_CONNECTION;
    }
    /*Проверка несиправности ЗРМ: неисправность АЦП, неисправность выпрямителя */
    if (Cfg.cmd.diag_wide) {
        if (pwm_st == STOP && !Cfg.mode.group) {
        /* Если преобразователь выключен и блок работает не в группе */
            if (get_adc_res(ADC_MU) > U_V(0,8) &&
                get_adc_res(ADC_MUp) < U_V(0,2)) {
                if (OUT_err_cnt < 250) OUT_err_cnt++;
            } else OUT_err_cnt = 0;
            /* если на выходе после реле напряжение есть,
               а до реле напряжения нет, значит КЗ выхода */
            if (OUT_err_cnt > 2) return ERR_OUT;
        }
        if (adc_error()) return ERR_ADC; /* не удаётся прочитать значение АЦП */
    }
    if (OVERLD) return ERR_OVERLOAD; /* проверка защиты от перегрузки */
    return NO_ERR;
}

static inline void csu_control (void) {
    csu_st pwm_st = pwm_state();
    if (pwm_st == STOP) return;
    if (!get_time_left(CntrlTime)) {
        CntrlTime = get_fin_time(CNTRL_T);
        uint16_t adc_i;
        uint16_t adc_u = get_adc_res(ADC_MU);
        int16_t err_i;
        int16_t err_u = TaskU - adc_u;
        if (pwm_st == CHARGE) {
            adc_i = get_adc_res(ADC_MI);
            err_i = TaskI - adc_i;
        } else { // discharge
            err_i = i_pwr_lim(Cfg.P_maxW, TaskId);
            err_i -= get_adc_res(ADC_DI);
        }
        if (!InitF) {
            SatU = false;
            InitF = true;
            Uerr = (float)err_u;
            Ierr = (float)err_i;
            if (Cfg.bf2.bit.pulse) {
                InfTau = 0;
                Pid_Ic = IcPidPlsDef;
                Pid_U = UPidPlsDef;
                Pid_Id = IdPidPlsDef;
            } else {
                InfTau = INF_TAU;
                Pid_Ic = IcPidDef;
                Pid_U = UPidDef;
                Pid_Id = IdPidDef;
            }
       } else {
            Uerr = flt_exp(Uerr, (float)err_u, InfTau);
            Ierr = flt_exp(Ierr, (float)err_i, InfTau);
        }
        if (pwm_st == CHARGE) {
            float err;
            if (Uerr > DIFF_U && Ierr > 0) {
                err = Ierr;
                InfTau = INF_TAU;
            } else {
                if (Uerr < Ierr) err = Uerr;
                else err = Ierr;
                if (adc_i < STABLE_I) {
                    InfTau = INF_TAU / K_FIN;
                    err *= K_FIN;
                    goto over_curr;
                } else {
                    InfTau = INF_TAU;
                }
            }
            PWM_I = pwm_duty(pid_r(&Pid_Ic, err), PWM_0I);
        over_curr:
            PWM_U = pwm_duty(pid_r(&Pid_U, err), PWM_0U);
        } else { // discharge
            if (SatU) {
                PWM_I = pwm_duty(pid_r(&Pid_Id, -Uerr), PWM_0D);
            } else { // !SatU
                if (Uerr > 0) SatU = true;
                PWM_I = pwm_duty(pid_r(&Pid_Id, Ierr), PWM_0D);
            }
        }      
        BreakTime = get_fin_time(SEC(1));
    }
}

static inline void check_auto_start (void) {
    if (Cfg.bf2.bit.astart) { // включён автостарт
        if (pwm_state() == STOP) { // преобразователь выключен
            if (AutoStr.err_cnt) {
            //количество перезапусков не исчерпано
                if (!get_time_left(AutoStr.rst_time)) {
                // истекло время паузы между запусками
                    if (AutoStr.u_pwm > get_adc_res(ADC_MU)) {
                    // напряжение на выходе меньше чем напряжение запуска
                        if (CsuState == STOP)
                            AutoStr.rst_time = get_fin_time(MS(Cfg.time_set));
                        if (Error) AutoStr.err_cnt--;
                        key_power();// нажата кнопка Старт/Стоп
                    }
                }
            }
        }
    }
}

static inline void update_led (void) {
    if (Error == ERR_CONNECTION) LED_POL(1);
    else LED_POL(0);
    if (CsuState != STOP) LED_PWR(1);
    if (pwm_state() == CHARGE) {
        if (I_ST) {
            LED_STI(1);
            LED_STU(0);
        } else {
            LED_STI(0);
            LED_STU(1);
        }
    } else {
        LED_STI(0);
        LED_STU(0);
    }
}

static inline void start_cntrl (void) {
    TickSec = get_fin_time(SEC(1));
    LedPwrTime = get_fin_time(PWR_TIME);
    CntrlTime = get_fin_time(CNTRL_T);
}

static void out_off (void) {
    RELAY_OFF();
    ALARM_OUT(ON);
    AlarmDel = get_fin_time(SEC(5));
}

static inline void read_tmp (void) {
    uint8_t err = get_tmp_res(Tmp);
    for (uint8_t i = 0; i < T_N; i++) {
        if (err & (i << T_ERROR)) {
            if (ErrT[i] < T_ERR_CNT) ErrT[i]++;
            else Tmp[i] = ERR_WCODE;
        } else ErrT[i] = 0;
    }
}

#pragma vector=INT1_vect
#pragma type_attribute=__interrupt
void int_1_ext (void) {
    stop_pwm(HARD);
    Error = ERR_OVERLOAD;
}