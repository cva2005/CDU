#pragma message	("@(#)ldc.c")
#include <system.h>
#include "spi/adc/ads1118.h"
#include "pwm/pwm.h"
#include "csu/csu.h"
#include "csu/mtd.h"
#include "key/key.h"
#include "lcd.h"

#ifndef JTAG_DBGU
static void decd_cpy (char *dst, const char *src, uint8_t n);
static void uint_to_str (uint16_t n, char *p, uint8_t dig);
static inline char dprn (unsigned char d);
static void calc_cap (int8_t sign, uint16_t val, uint16_t k, char *p);
static void calc_prm (uint16_t val, uint16_t k, char *p);
static void calc_tmp (int16_t t, char *p);
static void calc_time (hms_t *t, char *p);
static void tc_set (void);

static bool ConMsg;
static bool TickSec = false;
static char Lcd[LN][SL];
static const uint8_t LADDR[LN] = {LA_0, LA_1, LA_2, LA_3};
static lcd_st lcdState = IDLE;
uint16_t AdcOld[4];
int16_t TmpOld[T_N];
static int8_t cOld, sOld;
lcd_mode_t LcdMode = TIME_MODE;
cap_t Cap; // Capacity of Battery
static const char RuCh[64] = {
    0x41,0xA0,0x42,0xA1,0xE0,0x45,0xA3,0xA4,0xA5,0xA6,0x4B,0xA7,0x4D,0x48,0x4F,0xA8,
    0x50,0x43,0x54,0xA9,0xAA,0x58,0xE1,0xAB,0xAC,0xE2,0xAD,0xAE,0x62,0xAF,0xB0,0xB1,
    0x61,0xB2,0xB3,0xB4,0xE3,0x65,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0x6F,0xBE,
    0x70,0x63,0xBF,0x79,0xE4,0x78,0xE5,0xC0,0xC1,0xE6,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7
};

void lcd_tick_sec (void) {
    TickSec = true;
}

void lcd_start (void) {
    if (Cfg.mode.lcd) {
        lcd_wr_connect(false);
        lcd_wr_set();
    }
}

bool lcd_conn_msg (void) {
    return ConMsg;
}

void lcd_clear (void) {
    memset(Lcd, ' ', sizeof(Lcd));
}

void lcd_mode_ch (void) {
    if (LcdMode == TIME_MODE) {
        memcpy(&Lcd[3][0], "t1=", 3);
        memcpy(&Lcd[3][8], "; t2=", 5);
        memset(&Lcd[3][18], ' ', 2);
        TmpOld[0] = TmpOld[1] = ERR_WCODE;
        LcdMode = TEMP_MODE;
    } else { // TEMP_MODE
        tc_set();
        calc_time(&Tm, &Lcd[3][T_P]);
        calc_cap(0, get_adc_res(ADC_MI), Cfg.K_I, &Lcd[3][C_P]);
        LcdMode = TIME_MODE;
    }
    WH2004_string_wr(&Lcd[3][0], LA_3, 20);
};

void lcd_wr_set (void) {
    ConMsg = false;
    lcd_clear();
    memcpy(&Lcd[0][0], "P:", 2);
    decd_cpy(&Lcd[0][15], "Ц= /", 4);
    memcpy(&Lcd[1][0], "I=", 2);
    memcpy(&Lcd[1][7], "A(", 2);
    decd_cpy(&Lcd[1][13], "A)  Э:", 6);
    memcpy(&Lcd[2][0], "U=+", 3);
    memcpy(&Lcd[2][7], "B(", 2);
    memcpy(&Lcd[2][13], "B)", 2);
    tc_set();
    lcd_update_set();
}

void lcd_wr_connect (bool pc) {
    lcd_clear();
    memcpy(&Lcd[0][5], "KRON GROUP", 10);
    ConMsg = pc;
    if (pc) decd_cpy(Lcd[1], "ПК подключен...", 15);
    else decd_cpy(Lcd[1], "Инициализация...", 16);
    decd_cpy(Lcd[3], "ЗавN", 4);
    read_num(&Lcd[3][4]);
    memcpy(&Lcd[3][14], SOFTW_VER, 2);
    for (uint8_t i = 0; i < LN; i++)
        WH2004_string_wr(Lcd[i], LADDR[i], SL);
    WH2004_inst_wr(0x0C); // cursor off
}

#define mt Mtd.fld.end // metod time fields
void lcd_update_set (void) {
    uint8_t i;
    if (!Cfg.mode.lcd) return;	
    if (LcdMode == TEMP_MODE) lcd_mode_ch();
    /* отображение названия метода */
    memcpy(&Lcd[0][2], Mtd.fld.name, sizeof(Mtd.fld.name));
    /* отображение номера цикла */
    Lcd[0][19] = dprn(Mtd.fld.Cnt);
    Lcd[1][19] = dprn(Mtd.fld.NStg);
    /* отображение времени */
    calc_time(&mt, &Lcd[3][T_P]);
    /* отображение ёмкости */
    Cap.C = Cap.dC = 0;
    calc_cap(0, get_adc_res(ADC_MI), Cfg.K_I, &Lcd[3][C_P]);
    /* отображение тока и напряжения */
    if (SetMode == DISCHARGE) calc_prm(get_task_id(), Cfg.K_Id, &Lcd[1][Is_P]);
    else calc_prm(get_task_ic(), Cfg.K_I , &Lcd[1][Is_P]);
    calc_prm(get_task_u(), Cfg.K_U, &Lcd[2][Us_P]);
    for (i = 0; i < LN; i++) WH2004_string_wr(Lcd[i], LADDR[i], SL);
    WH2004_inst_wr(cursor_pos());
    WH2004_inst_wr(0x0F);//Display ON (D=1 diplay on, C=1 cursor on, B=1 blinking on)
    AdcOld[ADC_MU] = AdcOld[ADC_MI] = AdcOld[ADC_DI] =
    TmpOld[0] = TmpOld[1] = ERR_WCODE;
    cOld = sOld = UINT8_MAX;
}

void lcd_stop_msg (void) {
    decd_cpy(&Lcd[0][2], "Завершено    ", 13);
    WH2004_string_wr(&Lcd[0][2], LA_0 + 2, 13); // refrash
}

void lcd_update_work (void) {
    if (!Cfg.mode.lcd) return;
    WH2004_inst_wr(0x0C);//отключить курсор: Display ON (D=1 diplay on, C=0 cursor off, B=0 blinking off)
    csu_st pwm_st = pwm_state();
    /* ОТОБРАЖЕНИЕ ВЫХОДНОГО ТОКА */
    int16_t adc_res;
    if (pwm_st == DISCHARGE) { //если мы в режиме разряда, значит ток надо расчитывать по другому
        adc_res = get_adc_res(ADC_DI);
        if (adc_res != AdcOld[ADC_MI]) {
            Lcd[1][I_P]='-';
            AdcOld[ADC_MI] = adc_res;
            calc_prm(adc_res, Cfg.K_Id, &Lcd[1][I_P+1]);
            WH2004_string_wr(&Lcd[1][I_P],LA_1+I_P, 5); //отобразить
            calc_prm(get_task_id(), Cfg.K_Id , &Lcd[1][Is_P]);
            WH2004_string_wr(&Lcd[1][Is_P],LA_1+Is_P, 4); //отобразить
        }		
    } else {
        adc_res = get_adc_res(ADC_MI);
        if (adc_res != AdcOld[ADC_MI]) { //если значение тока изменилось
            Lcd[1][I_P]='+';
            AdcOld[ADC_MI] = adc_res;
            calc_prm(adc_res, Cfg.K_I, &Lcd[1][I_P+1]);
            WH2004_string_wr(&Lcd[1][I_P],LA_1+I_P, 5); //отобразить
            if (pwm_state() != STOP) { //Если блок не запущен, то не обнолвять заданные значения, т.к. они зависят от выбранного режима и обновляются в lcd_wr_set
                calc_prm(get_task_ic(), Cfg.K_I , &Lcd[1][Is_P]);
                WH2004_string_wr(&Lcd[1][Is_P],LA_1+Is_P, 4); //отобразить	
            }
        }
    }
    /* ОТОБРАЖЕНИЕ НОМЕРА ЭТАПА */
    if (CsuState != STOP) {
        if (sOld != sCnt) { //на последнем методе проверяется 
            Lcd[1][19] = dprn(sCnt + 1); //отображать значения на 1 больше, т.к. нумерация этапов начинается с 0
            WH2004_string_wr(&Lcd[1][19], LA_1 + 19, 1); //отобразить
            sOld = sCnt;
        }
    }
    /* ОТОБРАЖЕНИЕ ТЕКУЩЕГО ВРЕМЕНИ & CAPACITY */
    if (pwm_st != STOP) {
        if (TickSec == true) {		
            if (pwm_st == DISCHARGE)
                calc_cap(-1, get_adc_res(ADC_DI), Cfg.K_Id, &Lcd[3][C_P]);
            else calc_cap(1, get_adc_res(ADC_MI), Cfg.K_I, &Lcd[3][C_P]);
            if (LcdMode == TIME_MODE) {
                WH2004_string_wr(&Lcd[3][C_P],LA_3+C_P, 6);	 
                calc_time(&Tm, &Lcd[3][T_P]);
                WH2004_string_wr(&Lcd[3][T_P],LA_3+T_P, 8);
            }
            TickSec = false;
        }
    }	
    /* ОТОБРАЖЕНИЕ ТЕМПЕРАТУРЫ */
    if (LcdMode == TEMP_MODE) {
        for (uint8_t i = 0; i < T_N; i++) {
            int16_t tmp = get_csu_t((tch_t)i);
            if (TmpOld[i] != tmp) { //если значение изменилось, то отобразить его на дисплее
                uint8_t pos = i * T_DP + T1_P;
                calc_tmp(tmp, &Lcd[3][pos]); //преобразовать значение в цифры дисплея
                WH2004_string_wr(&Lcd[3][pos], LA_3 + pos, 5); //отобразить
                TmpOld[0] = tmp;
            }
        }
    }
    /* ОТОБРАЖЕНИЕ НАПРЯЖЕНИЯ */
    adc_res = get_adc_res(ADC_MU);
    if (adc_res != AdcOld[ADC_MU]) { //если значение напряжения изменилось
        AdcOld[ADC_MU] = adc_res;
        calc_prm(adc_res, Cfg.K_U, &Lcd[2][U_P]);
        WH2004_string_wr(&Lcd[2][U_P],LA_2+U_P, 5); //отобразить
        calc_prm(get_task_u(), Cfg.K_U , &Lcd[2][Us_P]);
        WH2004_string_wr(&Lcd[2][Us_P],LA_2+Us_P, 4); //отобразить
    }
    /* ОТОБРАЖЕНИЕ ПОДКЛЮЧЕНИЯ ПК */
    if (rs_active()) {
        if (Lcd[2][16] != P_rus) {
            decd_cpy(&Lcd[3][16], "ПК", 2);
            goto refr_pc_msg;
        }
    } else {
        if (Lcd[2][16]!=' ') {
            memset(&Lcd[2][16], ' ', 2);
        refr_pc_msg:
            WH2004_string_wr(&Lcd[2][16], LA_2 + 16, 2);
        }
    }
    /* ОТОБРАЖЕНИЕ НОМЕРА ЦИКЛА */
    if (cOld != cCnt) {
        uint_to_str(cCnt, &Lcd[0][S_P], 1);
        WH2004_string_wr(&Lcd[0][S_P], LA_0 + S_P, 1);
        cOld = cCnt;
    }
    /* -ОТОБРАЖЕНИЕ ОШИБОК */
    err_t err = get_csu_err();
    if (err) {
        memcpy(&Lcd[0][2], "Error", 5);
        uint_to_str(err, &Lcd[0][7], 2);
        memset(&Lcd[0][9], ' ', 6);
        WH2004_string_wr(&Lcd[0][2], LA_0+2, 13);
        memset(Lcd[3], ' ', 20);
        switch (err) {
        case ERR_OVERLOAD:
            decd_cpy(&Lcd[3][11], "заряда", 6);
            goto overl_msg;
        case ERR_DISCH_PWR:
            decd_cpy(&Lcd[3][11], "разряда", 7);
        overl_msg:
            decd_cpy(Lcd[3], "Перегрузка", 10);
            break;
        case ERR_CONNECTION:
        case ERR_CONNECTION1:
            decd_cpy(Lcd[3], "Переполюсовка", 13);
            break;
        case ERR_NO_AKB:
            decd_cpy(Lcd[3], "Нет АКБ!!!", 10);
            break;
        case ERR_OVERTEMP1:
            Lcd[3][16] = '1';
            goto overt_msg;
        case ERR_OVERTEMP2:
            Lcd[3][16] = '2';
            goto overt_msg;
        case ERR_OVERTEMP3:
            decd_cpy(&Lcd[3][16], "внеш", 4);
        overt_msg:
            decd_cpy(Lcd[3], "Перегрев датчик", 15);
            break;
        case ERR_SET:
            decd_cpy(Lcd[3], "Задано неверное U", 17);
            break;
        case ERR_STG:
            decd_cpy(Lcd[3], "Этап поврежден", 14);
            break;
        case ERR_OUT:
            decd_cpy(Lcd[3], "КЗ выпрямителя", 14);
            break;
        case ERR_ADC:
            decd_cpy(Lcd[3], "Ошибка АЦП", 10);
            break;
        case ERR_DM_LOSS:
            decd_cpy(Lcd[3], "Обрыв разряд. модуля", 20);
        }
        WH2004_string_wr(&Lcd[3][0], LA_3, 20);
    } else {
        if ((Lcd[3][0] != 'T') && (Lcd[3][0] != 't')) lcd_wr_set();
        /* ОТОБРАЖЕНИЕ БИТОВ СОСТОЯНИЯ */
        bool change = false;
        char sc;
        if (pwm_st == STOP) {
            if (lcdState != IDLE) {
                lcdState = IDLE;
                sc = ' ';
                change = true;
            }
        } else {
            if (pLim) {
                if (lcdState != PLIM) {
                    lcdState = PLIM;
                    sc = 'P';
                    change = true;
                }
            } else {
                if ((I_ST && pwm_st == CHARGE) ||
                    (pwm_st == DISCHARGE
                     && (get_adc_res(ADC_MU) > get_task_u() + 5))) {
                    if (lcdState != CURR) {
                        lcdState = CURR;
                        sc = 'I';
                        change = true;
                    }
                } else {
                    if (lcdState != VOLT) {
                        lcdState = VOLT;
                        sc = 'U';
                        change = true;
                    }
                }
            }
        }
        if (change) {
            Lcd[2][19] = sc;
            WH2004_string_wr(&sc, LA_2 + 19, 1);
        }
    }
    /* вернуть курсор на место */
    WH2004_inst_wr(cursor_pos());
    if (CsuState == STOP) WH2004_inst_wr(0x0F);
    /* Display ON (D=1 diplay on, C=1 cursor on, B=1 blinking on) */
}

#define TBL_NUM 0xC0
static void decd_cpy (char *dst, const char *src, uint8_t n) {
    char ch;
    while (n--) {
        ch = *src++;
        if (ch >= TBL_NUM) *dst = RuCh[ch - TBL_NUM];
        *dst++ = ch;
    }
}

static void uint_to_str (uint16_t n, char *p, uint8_t dig) {
    while (dig--) {
        p[dig] = n % 10 + 0x30;
        n /= 10;
    }
}

static inline char dprn (unsigned char d) {
    if (d > 9) return ('x');
    return (d + 0x30);
}

// ToDo: check time interval!
static void calc_cap (int8_t sign, uint16_t val, uint16_t k, char *p) {
    if (sign != 0) {
        uint32_t cur = val * k / D10K * 2778UL; //+(uint32_t)b;
        cur = cur / D100K;
        Cap.dC += cur * sign;
        if (Cap.dC > D10K) {
            Cap.C++;
            Cap.dC -= D10K;
        } else if (Cap.dC < -D10K) {
            Cap.C--;
            Cap.dC += D10K;
        }		
    }
    if (Cap.C < 0) {
        p[0] = '-';
        val = (-1) * Cap.C;	
    } else {
        p[0] = '+';
        val = Cap.C;
    }
    uint_to_str(val, &p[1], 4);
    p[5] = p[4];
    p[4] = ',';
}

static void calc_prm (uint16_t val, uint16_t k, char *p) {
	uint32_t cur = val * k; //+(uint32_t)b;
    val = (cur % D1M) / 100;
    cur /= D1M;
    if (val >= 5) cur++; // mathematical rounding
    uint_to_str(val, p, 3);
    p[3] = p[2];
    p[2] = ',';
}

/* Temperature range default 0.0625°C / bit */
/*signed short tmp = (T2 << 8) + T1;
return double(tmp) * 0.0625;*/
static void calc_tmp (int16_t t, char *p) {
    if (t == ERR_WCODE) {
        memcpy(p, "Error", 5);
        return;
    }
    if (t < 0) {
        p[0]='-';
        t *= -1;
    }
    uint16_t d = t * 5 / 8; // T[°C] * 10
    if (d > 999) {
        p[0] = '+';
        uint_to_str(d, &p[1], 3);
    } else {
        uint_to_str(d, p, 4);
    }
    p[4] = p[3];
    p[3] = ',';
}

static void calc_time (hms_t *t, char *p) {
    if (t->h < 100 && t->m < 60 && t->s < 60) {
        uint_to_str (t->h, &p[0], 2);
        uint_to_str (t->m, &p[3], 2);
        uint_to_str (t->s, &p[6], 2);
        p[2] = p[5] = ':';
    } else {
        memcpy(p, "--:--:--", 8);
    }
}

static void tc_set (void) {
    memcpy(&Lcd[3][0], "T=", 2);
    memcpy(&Lcd[3][10], "; C=", 4);
}
#endif // #ifndef JTAG_DBGU