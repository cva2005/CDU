#pragma message	("@(#)csu.c")

#include <system.h>
#include "csu/csu.h"
#include "csu/mtd.h"
#include "lcd/wh2004.h"
#include "lcd/lcd.h"
#include "pwm/pwm.h"
#include "spi/adc/ads1118.h"
#include "pid/pid_r.h"
#include "tsens/ds1820.h"

stime_t AlarmDel;
stime_t BreakTime;
stime_t TickSec;
stime_t LedPwrTime;
ast_t ast;
uint16_t id_dw_Clb, id_up_Clb;
state_t State;
float Uerr, Ierr, Usrt;
unsigned int  dm_loss_cnt = 0;
unsigned char ERR_Ext = 0, OUT_err_cnt = 0;
bool InitF, SatU, Stable, DownMode, PulseMode;
bool pLim = false, LedPwr;
unsigned int StbCnt;
csu_st CsuState = STOP;
unsigned char ZR_mode = 1, Error = 0;
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

static inline void pid_init_all (void);
static void out_off (void);

void Correct_UI (void) {
    if (ADS1118_St[ADC_MU] &&
        (ADS1118_St[ADC_MI] || ADS1118_St[ADC_DI])) {
        ADS1118_St[ADC_MU] = 0;
        int err_i;
        int err_u = set_U - ADC_ADS1118[ADC_MU].word;
        if (ADS1118_St[ADC_MI]) {
            ADS1118_St[ADC_MI] = 0;
            err_i = set_I - ADC_ADS1118[ADC_MI].word;
        } else {
            ADS1118_St[ADC_DI] = 0;
            err_i = i_power_limit(Cfg.P_maxW, set_Id);
            err_i -= ADC_ADS1118[ADC_DI].word;
        }
        if (PwmStatus == CHARGE) {
            if (State == DISCHARGE_ST) {
                State = CHARGE_ST;
                goto pulse_mode;
            }
        } else if (PwmStatus == DISCHARGE) {
            if (State == CHARGE_ST) {
                State = DISCHARGE_ST;
                pulse_mode:
                PulseMode = true;
                pid_init(&Pid_Ic);
                pid_init(&Pid_Id);
            }        
        } else return;
        if (PulseMode || !InitF) {
            InitF = true;
            Usrt = Uerr = (float)err_u;
            Ierr = (float)err_i;
        } else {
            Uerr = Uerr * (1 - 1.0 / INF_TAU) + (float)err_u * (1.0 / INF_TAU);
            Ierr = Ierr * (1 - 1.0 / INF_TAU) + (float)err_i * (1.0 / INF_TAU);
        }
        if (PwmStatus == CHARGE) {
            /*if (!Stable) {
                if (StbCnt < ST_TIME) {
                StbCnt++;
            } else {
                Stable = true;
                if (Usrt - Uerr > DOWN_LIM) DownMode = true;
            }
            }*/
            float tmp;
            /*if (SatU) {
                tmp = Uerr;
            } else { // !SatU
                if (Uerr <= 0) SatU = true;
                tmp = Ierr;
            }
            if (!PulseMode && DownMode) {
                if (tmp > CURR_DT) tmp = CURR_DT;
                else if (tmp < -CURR_DT) tmp = -CURR_DT;
            }*/
            if (Uerr <= 0) tmp = Uerr;
            else tmp = Ierr;
            P_wdI = PwmDuty(pid_r(&Pid_Ic, tmp));
            P_wdU = PwmDuty(pid_r(&Pid_U, Uerr));
        } else { // discharge
            if (SatU) {
                P_wdI = PwmDuty(pid_r(&Pid_Id,  -Uerr));
            } else { // !SatU
                if (Uerr > 0) SatU = true;
                P_wdI = PwmDuty(pid_r(&Pid_Id, Ierr));
            }
        }      
        BreakTime = get_fin_time(SEC(1));
    }
}

void init_cntrl (void) {
    DownMode = Stable = SatU = InitF = false;
    StbCnt = 0;
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
    ast.u_pwm = ((uint32_t)ast.u_set*100000UL) / Cfg.K_U;
    ast.restart_cnt = ast.cnt_set;
    ast.restart_time = ast.time_set;
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
    max_pwd_Id = calculate_pwd((max_set_Id + (max_set_Id / 10)), 0);
    LCD_clear();
    if (Cfg.bf1.LCD_ON) Init_WH2004(1);
    else Init_WH2004(0);	
    if (Cfg.bf1.LED_ON) {
        DDRC = 0xFF;
        PORTC = 0xFF;	
        LedPwrTime = get_fin_time(PWR_TIME);
	}
}

void Start_CSU (csu_st mode) {
    unsigned char control_setU = 0;
    err_check();
    if (Error) return;
#if PID_CONTROL
    if (!CsuState) {
        PulseMode = true;
        pid_init_all();
    }
#endif
    if (CsuState == STOP && set_U) control_setU = 1;
    else control_setU = 0;
    if (PwmStatus!=0) {
        Stop_PWM(SOFT);
        pLim = false;
    }
    CsuState = (csu_st)(mode & 0x0F);
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
#if !PID_CONTROL
        if (Cfg.bf1.LED_ON && !RsActive) {
            if (ADC_ADS1118[ADC_MU].word>U_V(16,0)) set_U=U_V(31,0);
            else set_U=U_V(16,0);
            set_I=I_A(1,0);
        }
#endif
        soft_start(Cfg.bf1.DIAG_WIDE&&control_setU&&(Cfg.bf1.GroupM==0));
        ADC_ADS1118[ADC_MI].word=0;
    }
    if (CsuState == DISCHARGE) {
        soft_start_disch();	
        ADC_ADS1118[ADC_DI].word=0;
    }
    BreakTime = get_fin_time(SEC(1));
}

void Stop_CSU(csu_st mode) {
    if (!Error) ast.restart_cnt = AUTOSTART_CNT;
    Error = 0;
    Stop_PWM(SOFT);
    pLim = false;
    CsuState = STOP;
    delay_ms(10);
    if (mode == STOP) out_off();
    else RELAY_ON;
}

void err_check (void) {
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
                if (ADC_ADS1118[ADC_DI].word > Id_A(0,1)) {
                    if ((set_U < ADC_ADS1118[ADC_MU].word) && !pLim
                        && (set_Id > (ADC_ADS1118[ADC_DI].word << 1))) {
                        Error = ERR_DM_LOSS;
                    }
                    if ((set_Id > (ADC_ADS1118[ADC_DI].word + Id_A(0,2))) &&
                        (P_wdI == max_pwd_Id)) {
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
            if (ADC_ADS1118[ADC_MI].word >= I_A(0,1)) goto set_break_time;
            if (get_time_left(BreakTime) && (P_wdI > 0)) Error = ERR_NO_AKB;
        }
        if ((PwmStatus == DISCHARGE) && set_Id) {
            if (ADC_ADS1118[ADC_DI].word >= Id_A(0,1)) {
            set_break_time:
                BreakTime = get_fin_time(SEC(1));
            }
            if (get_time_left(BreakTime) && (P_wdI > 0)) Error = ERR_NO_AKB;
        }
    }
    /* Проверка перегрева */
    if (PwmStatus == DISCHARGE) {
        goto check_over_t2;
    } else if (PwmStatus == CHARGE) {
        if ((Temp1.fld.V > MAX_T1) && (Temp1.word != ERR_WCODE))
            /* проверка перегрева транзисторов */
            Error=ERR_OVERTEMP1;
        check_over_t2:
        if ((Temp2.fld.V > MAX_T2ch) && (Temp2.word != ERR_WCODE))
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
            if ((ADS1118_St[ADC_MUp]!=0)&&(ADS1118_St[ADC_MU]!=0)) {
            /* есть данные о напряжении */
                if ((ADC_ADS1118[ADC_MU].word > U_V(0,8)) &&
                    (ADC_ADS1118[ADC_MUp].word < U_V(0,2))) {
                    if (OUT_err_cnt < 250) OUT_err_cnt++;
                } else OUT_err_cnt = 0;
                /* если на выходе после реле напряжение есть,
                   а до реле напряжения нет, значит КЗ выхода */
                if (OUT_err_cnt > 2) Error = ERR_OUT;
                if (!OUT_err_cnt && (Error == ERR_OUT)) Error = 0;
                ADS1118_St[ADC_MU] = 0;
                ADS1118_St[ADC_MUp] = 0;
            }
            /* Если данные конфигурации АЦП неверные значит АЦП неисправен */
            if (!ADC_cfg_rd.word || (ADC_cfg_rd.word == ERR_WCODE)) Error = ERR_ADC;
        }
        if (!ADC_wait) Error = ERR_ADC; /* не удаётся прочитать значение АЦП */
    }
    if (Overload) Error = ERR_OVERLOAD; /* проверка защиты от перегрузки */
    if (Error) {
        /* если преобразователь не выключен: выключить преобразователь */
        if (PwmStatus != STOP) Stop_PWM(HARD);
        if (RELAY_EN) out_off();
    }
}

void csu_time_drv (void) {
    /*if (get_time_left(BreakTime)) {
        BreakTime = get_fin_time(SEC(1));
    }*/
    if (PwmStatus != STOP && !get_time_left(TickSec)) {
        TickSec = get_fin_time(SEC(1));
        Sec++; // секунды метода
        Sec_Stg++; // секунды этапа
        if (Sec > 59) {
            Min++; // минуты метода
            Sec = 0;
        }
        if (Min > 59) {
            if (Hour < 255) Hour++;	//Часы метода
            Min = 0;
        }
        if (Sec_Stg > 59) {
            Min_Stg++; // Минты этапа
            Sec_Stg = 0;
        }
        if (Min_Stg > 59) {
            Hour_Stg++;	//Часы этапа
            Min_Stg = 0;
        }
    }
    /* Индикация ошибок для LED */
    if (Cfg.bf1.LED_ON && CsuState == STOP && !get_time_left(LedPwrTime)) {
        LedPwrTime = get_fin_time(PWR_TIME);
        LED_PWR(LedPwr);
        LedPwr = !LedPwr;
    }
}

static inline void pid_init_all (void) {
    pid_init(&Pid_U);
    pid_init(&Pid_Ic);
    pid_init(&Pid_Id);
    TickSec = get_fin_time(SEC(1));
    LedPwrTime = get_fin_time(PWR_TIME);
}

static void out_off (void) {
    RELAY_OFF;
    ALARM_OUT(1);
    AlarmDel = get_fin_time(SEC(5));
}

#pragma vector=INT1_vect
#pragma type_attribute=__interrupt
void int_1_ext (void) {
    Stop_PWM(0);
    Error = ERR_OVERLOAD;
}