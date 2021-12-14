#pragma message	("@(#)csu.c")

#include <system.h>
#include "csu/csu.h"
#include "csu/mtd.h"
#include "lcd/wh2004.h"
#include "lcd/lcd.h"
#include "pwm/pwm.h"
#include "spi/adc/ads1118.h"
#include "pid/pid_r.h"

ast_t ast;
uint16_t id_dw_Clb, id_up_Clb;
state_t State;
float Uerr, Ierr, Usrt;
bool InitF, SatU, Stable, DownMode, PulseMode;
unsigned int StbCnt;
unsigned char CSU_Enable = 0, ZR_mode = 1, Error = 0, p_limit = 0;
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
        if (PWM_status == CHARGE) {
            if (State == DISCHARGE_ST) {
                State = CHARGE_ST;
                goto pulse_mode;
            }
        } else if (PWM_status == DISCHARGE) {
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
        if (PWM_status == CHARGE) {
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
        No_akb_cnt = NO_BATT_TIME;
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
	}
}
