#pragma message	("@(#)csu.c")
#include <system.h>
#include "lcd/lcd.h"
#include "pwm/pwm.h"
#include "spi/adc/ads1118.h"
#include "csu/mtd.h"
#include "pid/pid_r.h"
#include "tsens/ds1820.h"
#include "key/key.h"
#include "csu/csu.h"

static inline void check_auto_start (void);
static inline void csu_control (void);
static inline void start_cntrl (void);
static inline void update_led (void);
static inline void err_check (void);
static inline void read_tmp (void);
static void out_off (void);

stime_t LcdRefr, AlarmDel, FanTime;
static uint8_t ErrT[TCH] = {0, 0}; /* error count for T sensors */
static stime_t BreakTime, TickSec, LedPwrTime, CntrlTime;
ast_t AutoStr;
uint16_t id_dw_Clb, id_up_Clb;
uint16_t ADC_O[ADC_CH]; //данные АЦП без изменений (бе вычета коэфициента В)
static float Uerr, Ierr;
unsigned int  dm_loss_cnt = 0;
unsigned char ERR_Ext = 0, OUT_err_cnt = 0;
static bool InitF, SatU;
bool pLim = false, LedPwr;
bool SelfCtrl = false; //упр-е мет. заряда самостоятельно или удалённо
unsigned int StbCnt;
unsigned char Error = 0;
csu_st CsuState, SetMode = STOP;
static pid_t Pid_U = {
	0.0012, /* Kp; gain factor */
	5000.0, /* Ti integration time */
	10.0,   /* Tf derivative filter tau */
	20.0,   /* Td derivative time */
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
static pid_t Pid_Ic = {
	0.0001, /* Kp; gain factor */
	1000.0, /* Ti integration time */
	10.0,   /* Tf derivative filter tau */
	2.0,    /* Td derivative time */
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
static pid_t Pid_Id = {
	0.0001, /* Kp; gain factor */
	1000.0, /* Ti integration time */
	10.0,   /* Tf derivative filter tau */
	2.0,    /* Td derivative time */
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
    if (PwmStatus != STOP && !get_time_left(TickSec)) {
        TickSec = get_fin_time(SEC(1));
        CapCalc = false;
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
    if (Cfg.bf1.LED_ON && CsuState == STOP
        && !get_time_left(LedPwrTime)) {
        LedPwrTime = get_fin_time(PWR_TIME);
        LED_PWR(LedPwr);
        LedPwr = !LedPwr;
    }
    /* FAN control section */
    if (!get_time_left(FanTime)) {
        FanTime = get_fin_time(MS(500));
        read_tmp();
        if (!Cfg.bf1.FAN_CONTROL) {
            int16_t t1 = Tmp[0]; int16_t t2 = Tmp[1]; 
            if (t1 > FAN_ON_T || t2 > FAN_ON_T ||
                (t1 >= -FAN_CND_T && t1 < FAN_CND_T) ||
                (t2 >= -FAN_CND_T && t2 < FAN_CND_T) ||
                t1 == ERR_WCODE || t2 == ERR_WCODE) FAN(1);
            else if (t1 < FAN_OFF_T && t2 < FAN_OFF_T) FAN(0);
        }
    }
    err_check();
    if (!Cfg.bf1.LED_ON || rs_active()) csu_control();
    if (Clb.id.bit.save == 1) {
        if (!Clb.id.bit.error
            && (Clb.id.bit.dw_Fin || Clb.id.bit.up_Fin)) {
            save_clb();
            Clb.id.byte = 0;
        }
    }
    if (Cfg.bf1.LCD_ON) {
        if (!rs_active()) { //если подключение прервалось, а значения дисплея не обновлены
            if (lsd_conn_msg()) {
                if (Cfg.bf1.DIAG_WIDE) {
                    csu_stop(STOP);
                    read_mtd();
                }
                lcd_wr_set();
            }				
        } else	{ //если подключение появилось, а значения дисплея не обновлены
            if (!Cfg.bf1.DEBUG_ON && !lsd_conn_msg()) lcd_wr_connect(true);
        }			
    }
    if (!rs_active() || Cfg.bf1.DEBUG_ON) {
        if (Cfg.bf1.LED_ON)	update_led();
        if (Cfg.bf1.LCD_ON) {
            if (!get_time_left(LcdRefr)) {
                lsd_update_work();
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
            ALARM_OUT(0);
        }
    }
}

void calc_cfg (void) {
    Error = 0;
    if (Cfg.maxI > 5000) Cfg.maxI = 5000;
    max_set_I = (uint32_t)Cfg.maxI * 100000UL / Cfg.K_I;
    if (Cfg.bf1.GroupM) max_set_I = max_set_I + I_A(1,0);
    if (Cfg.dmSlave == 0) {
        if (Cfg.maxId > maxId_EXT0) Cfg.maxId = maxId_EXT0;
    } else {
        if (Cfg.maxId > maxId_EXT12) Cfg.maxId = maxId_EXT12;
    }
    max_set_Id = (uint32_t)Cfg.maxId * 100000UL / Cfg.K_Id;
    if (Cfg.bf1.GroupM) max_set_Id = max_set_Id + I_A(1,0);
    if (Cfg.maxU > 4000) Cfg.maxU = 4000;
    max_set_U = (uint32_t)Cfg.maxU * 100000UL / Cfg.K_U;
    max_pwd_I = (uint32_t)max_set_I * 100UL / K_PWD_I + B_PWD_I;
    if (max_pwd_I < 85) max_pwd_I = 85;
    max_pwd_U = (uint32_t)max_set_U * 100UL / K_PWD_U + B_PWD_U;
    if (Cfg.dmSlave > 2) Cfg.dmSlave = 2;
    if (Cfg.dmSlave > 0) Cfg.bf1.EXT_Id = 1;
    else Cfg.bf1.EXT_Id = 0;
    ADC_O[ADC_MU] = Cfg.B[ADC_MU];
    ADC_O[ADC_MI] = Cfg.B[ADC_MI];
    AutoStr.u_pwm = ((uint32_t)AutoStr.u_set*100000UL) / Cfg.K_U;
    AutoStr.err_cnt = Cfg.cnt_set;
    if (Cfg.bf2.astart) Cfg.bf1.DEBUG_ON = 1;
    id_dw_Clb = Id_A(2,0);
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
    if (id_up_Clb > max_set_Id) id_up_Clb = max_set_Id >> 1;
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
    max_pwd_Id = calc_pwd((max_set_Id + (max_set_Id / 10)), 0);
    lsd_clear();
    if (Cfg.bf1.LCD_ON) Init_WH2004(1);
    else Init_WH2004(0);	
    if (Cfg.bf1.LED_ON) {
        DDRC = 0xFF;
        PORTC = 0xFF;	
        LedPwrTime = get_fin_time(PWR_TIME);
	}
}

void csu_start (csu_st mode) {
    unsigned char control_setU = 0;
    err_check();
    if (Error) return;
    if (CsuState == STOP) {
         start_cntrl();
    }
    if (CsuState == STOP && set_U) control_setU = 1;
    else control_setU = 0;
    if (PwmStatus!=0) {
        Stop_PWM(SOFT);
        pLim = false;
    }
    CsuState = mode;
    if (CsuState == PAUSE) {
        if (RELAY_EN) out_off();
        return;
    }
    if (CsuState == DISCHARGE) DE(1);
    if (!RELAY_EN) {
        RELAY_ON;
        delay_ms(100);
    }
    if (CsuState == CHARGE) {
        soft_start(Cfg.bf1.DIAG_WIDE&&control_setU&&(Cfg.bf1.GroupM==0));
    }
    if (CsuState == DISCHARGE) {
        soft_start_disch();	
    }
    BreakTime = get_fin_time(SEC(1));
}

void csu_stop (csu_st mode) {
    if (!Error) AutoStr.err_cnt = Cfg.cnt_set;
    Error = 0;
    Stop_PWM(SOFT);
    InitF = pLim = false;
    CsuState = STOP;
    delay_ms(10);
    if (mode == STOP) out_off();
    else RELAY_ON;
}

uint16_t i_pwr_lim (uint16_t p, uint16_t i) {
    uint32_t u, id;
    pLim = false;
    u = get_adc_res(ADC_MU) * Cfg.K_U / D1M;	
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

static inline void err_check (void) {
    /* Проверка ошибки по внешнему входу */
    if ((Cfg.bf1.LED_ON==0)&&(Cfg.bf1.EXT_Id==1)) {
    /* индикация не светодиодная и включено управление вншним разрядным модулем */
        if ((OverTempExt && 1) != (Cfg.bf1.EXTt_pol && 1)) ERR_Ext++;
        /* проверка срабатывания внешнего термореле */
        else ERR_Ext = 0;
        if (ERR_Ext > EXT_ERR_VAL) Error = ERR_OVERTEMP3;
    }
    /* Проверка на обрыв РМ */
	if (Cfg.dmSlave) {
		if (Cfg.bf1.DIAG_WIDE && (PwmStatus == DISCHARGE)) {
        /* включена расширеная диагностика */
            if ((dm_loss_cnt > 0) && (dm_loss_cnt < 10)) {
                if (get_adc_res(ADC_DI) > Id_A(0,1)) {
                    if ((set_U < get_adc_res(ADC_MU)) && !pLim
                        && (set_Id > (get_adc_res(ADC_DI) << 1))) {
                        Error = ERR_DM_LOSS;
                    }
                    if ((set_Id > (get_adc_res(ADC_DI) + Id_A(0,2))) &&
                        (PWM_I == max_pwd_Id)) {
                        /* реальный ток в 2 раза меньше заданного */
                        Error = ERR_DM_LOSS;
                    }
                }
            }
        }
    }
    /* Проверка на обрыв нагрузки */
    if (Cfg.bf1.I0_SENSE && !Cfg.bf1.GroupM) {
         /* диагностика обрыва нагрузки и блок не в группе */
        if ((PwmStatus == CHARGE) && set_I) {
            if (get_adc_res(ADC_MI) >= I_A(0,1)) goto set_break_time;
            if (get_time_left(BreakTime) && (PWM_I > 0)) Error = ERR_NO_AKB;
        }
        if ((PwmStatus == DISCHARGE) && set_Id) {
            if ((ADC_DI) >= Id_A(0,1)) {
            set_break_time:
                BreakTime = get_fin_time(SEC(1));
            }
            if (get_time_left(BreakTime) && (PWM_I > 0)) Error = ERR_NO_AKB;
        }
    }
    /* Проверка перегрева */
    if (PwmStatus == DISCHARGE) {
        goto check_over_t2;
    } else if (PwmStatus == CHARGE) {
        if ((Tmp[0] > MAX_T1) && (Tmp[0] != ERR_WCODE))
            /* проверка перегрева транзисторов */
            Error = ERR_OVERTEMP1;
        check_over_t2:
        if ((Tmp[1] > MAX_T2ch) && (Tmp[1] != ERR_WCODE))
            /* проверка перегрева выпрямительных диодов */
        Error = ERR_OVERTEMP2;
    }
    /* Проверка полярности подключения АКБ (переполюсовка) */
    if (!Cfg.bf1.GroupM) {
        if (ADC_O[ADC_MU] < Cfg.B[ADC_MU]) {
            if ((Cfg.B[ADC_MU] - ADC_O[ADC_MU]) > 300) Error = ERR_CONNECTION;
            else if (Error == ERR_CONNECTION) Error = 0;
        } else if (Error == ERR_CONNECTION) Error = 0;
    }
    /*Проверка несиправности ЗРМ: неисправность АЦП, неисправность выпрямителя */
    if (Cfg.bf1.DIAG_WIDE) {
        if ((PwmStatus==STOP)&&(Cfg.bf1.GroupM==0)) {
        /* Если преобразователь выключен и блок работает не в группе */
            if ((get_adc_res(ADC_MU) > U_V(0,8)) &&
                (get_adc_res(ADC_MUp) < U_V(0,2))) {
                if (OUT_err_cnt < 250) OUT_err_cnt++;
            } else OUT_err_cnt = 0;
            /* если на выходе после реле напряжение есть,
               а до реле напряжения нет, значит КЗ выхода */
            if (OUT_err_cnt > 2) Error = ERR_OUT;
            if (!OUT_err_cnt && (Error == ERR_OUT)) Error = 0;
            /* Если данные конфигурации АЦП неверные значит АЦП неисправен */
            //if (!ADC_cfg_rd.word || (ADC_cfg_rd.word == ERR_WCODE)) Error = ERR_ADC;
        }
        if (adc_error()) Error = ERR_ADC; /* не удаётся прочитать значение АЦП */
    }
    if (Overload) Error = ERR_OVERLOAD; /* проверка защиты от перегрузки */
    if (Error) {
        /* если преобразователь не выключен: выключить преобразователь */
        if (PwmStatus != STOP) Stop_PWM(HARD);
        if (RELAY_EN) out_off();
    }
}

static inline void csu_control (void) {
    if (PwmStatus == STOP) return;
    if (!get_time_left(CntrlTime)) {
        CntrlTime = get_fin_time(CNTRL_T);
        uint16_t err_i;
        uint16_t err_u = set_U - get_adc_res(ADC_MU);
        if (PwmStatus == CHARGE) {
            err_i = set_I - get_adc_res(ADC_MI);
        } else { // discharge
            err_i = i_pwr_lim(Cfg.P_maxW, set_Id);
            err_i -= get_adc_res(ADC_DI);
        }
        if (/*Stg.fld.type == PULSE ||*/ !InitF) {
            InitF = true;
            Uerr = (float)err_u;
            Ierr = (float)err_i;
        } else {
            Uerr = Uerr * (1 - 1.0 / INF_TAU) + (float)err_u * (1.0 / INF_TAU);
            Ierr = Ierr * (1 - 1.0 / INF_TAU) + (float)err_i * (1.0 / INF_TAU);
        }
        if (PwmStatus == CHARGE) {
            float tmp;
            if (Uerr <= 0) tmp = Uerr;
            else tmp = Ierr;
            PWM_I = PwmDuty(pid_r(&Pid_Ic, tmp));
            PWM_U = PwmDuty(pid_r(&Pid_U, Uerr));
        } else { // discharge
            if (SatU) {
                PWM_I = PwmDuty(pid_r(&Pid_Id,  -Uerr));
            } else { // !SatU
                if (Uerr > 0) SatU = true;
                PWM_I = PwmDuty(pid_r(&Pid_Id, Ierr));
            }
        }      
        BreakTime = get_fin_time(SEC(1));
    }
}

static inline void check_auto_start (void) {
    if (Cfg.bf2.astart) { // включён автостарт
        if (PwmStatus == STOP) { // преобразователь выключен
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
    if (PwmStatus == CHARGE) {
        if (I_St) {
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
    pid_r_init(&Pid_U);
    pid_r_init(&Pid_Ic);
    pid_r_init(&Pid_Id);
    TickSec = get_fin_time(SEC(1));
    LedPwrTime = get_fin_time(PWR_TIME);
    CntrlTime = get_fin_time(CNTRL_T);
}

static void out_off (void) {
    RELAY_OFF;
    ALARM_OUT(1);
    AlarmDel = get_fin_time(SEC(5));
}

static inline void read_tmp (void) {
    uint8_t err = get_tmp_res(Tmp);
    uint8_t mask = T1_ERROR;
    for (uint8_t i = 0; i < TCH; i++) {
        if (err & mask) {
            if (ErrT[i] < READ_ERR_CNT) ErrT[i]++;
            else Tmp[i] = ERR_WCODE;
        } else ErrT[i] = 0;
        mask <<= 1;
    }
    tmp_convert();
}

#pragma vector=INT1_vect
#pragma type_attribute=__interrupt
void int_1_ext (void) {
    Stop_PWM(0);
    Error = ERR_OVERLOAD;
}