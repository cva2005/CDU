#pragma message	("@(#)key.c")
#include "sys/config.h"
#include "lcd/lcd.h"
#include "pwm/pwm.h"
#include "key.h"

static inline void key_set (void);
static void key_up_dw (up_dw_t up_dw);
static inline void key_power_led (void);
static inline void key_U_up (void);
static inline void key_U_dw (void);
static inline void key_I_up (void);
static inline void key_I_dw (void);
static void change_ic (int8_t add);
static void change_id (int8_t add);
static void change_u (int8_t add);

static stime_t KeyDel, KeyPress;
static uint8_t Step = 1, iCurs = 0;

uint8_t curs_idx (void) {
    return iCurs;
}

void check_key (void) {
    uint8_t key, mask;
    if (Cfg.mode.ext_id) mask = 0x08; // no scan K1
    else mask = 0;
    key = KEY_MASK | mask; // scan keys
    if (key != ALL_OFF) { // key press!
        for (uint8_t cnt = 0; cnt < 32; cnt++) { // stable cycle
            if (key != (KEY_MASK | mask)) goto no_press;
        }
        if (!get_time_left(KeyDel)) { //если разрешена обработка кнопок
            if (Cfg.mode.lcd) {
                if (!get_time_left(KeyPress)) { // клавишу удерживали нажатой
                    KeyPress = get_fin_time(MS(3200));
                    if (Step == 1) Step = 10;
                    else Step = 20;
                }
                if (key == K5) key_power(); // кнопка Старт/Стоп
                if (key == K4) key_set(); // кнопка Set
                if (key == K3) key_up_dw(UP_PRESS); // кнопка вверх
                if (key == K2) key_up_dw(DOWN_PRESS); // кнопка вниз
            }
            if (Cfg.mode.led) {
                if (!get_time_left(KeyPress)) { // клавишу удерживали нажатой
                    KeyPress = get_fin_time(MS(1600));
                    if (Step == 1) Step = 5;
                    else Step = 10;
                }
                if (key == K5) key_power_led();
                if (key == K4) key_U_up();
                if (key == K3) key_U_dw();
                if (key == K2) key_I_up();
                if (key == K1) key_I_dw();
            }
        }
    } else {
    no_press:
        KeyPress = get_fin_time(MS(1600));
        Step = 1;
    }
}

void key_power (void) {
    if (CsuState == STOP && !get_csu_err()) {
        SelfCtrl = true;
        start_mtd(1);
    } else {
        csu_stop(STOP);
        read_mtd();
        lcd_update_set();
    }
    KeyDel = get_fin_time(MS(800));
}

static inline void key_set (void) {
    if (CsuState == STOP) {
        iCurs++;
        if (iCurs == PR_NUM) iCurs = 0;
    } else {
        iCurs = 0;
        lcd_mode_ch();
    }
    lcd_set_curs(iCurs);
    KeyDel = get_fin_time(MS(320));
}

static void key_up_dw (up_dw_t up_dw) {
    if (CsuState == STOP) {
        SaveMtd = true;
        int8_t stp = Step * (int8_t)up_dw;
        switch (iCurs) {
        case IDX_MODE:
            mCnt += (int8_t)up_dw;
            read_mtd();
            SaveMtd = false;
            goto set_del_320;
        case IDX_I:
            if (SetMode == DISCHARGE) change_id(stp);
            else change_ic(stp);
            break;
        case IDX_U:
            change_u(stp);
            break;
        case IDX_TIME:
            if (up_dw == UP_PRESS) {
                if (Mtd.fld.end.h < 100) {
                    Mtd.fld.end.m += Step;
                    if (Mtd.fld.end.m > 59) {
                        Mtd.fld.end.h++;
                        Mtd.fld.end.m = 0;
                    }
                }
            } else { // DOWN_PRESS
                Mtd.fld.end.m -= Step;
                if (Mtd.fld.end.m > 59) {
                    if (Mtd.fld.end.h > 0) Mtd.fld.end.h--;
                    Mtd.fld.end.m = 59;
                }
            }
            KeyDel = get_fin_time(MS(160));
            break;
        case IDX_CYCLE:
            Mtd.fld.Cnt += (int8_t)up_dw;
            if (Mtd.fld.Cnt > 10) Mtd.fld.Cnt = 1;
            else if (Mtd.fld.Cnt < 1) Mtd.fld.Cnt = 10;
        set_del_320:
            KeyDel = get_fin_time(MS(320));
        }
        lcd_update_set();
    } else {
        if (SetMode == DISCHARGE) {
            change_id((int8_t)up_dw);
        } else {
            if (I_ST) change_ic((int8_t)up_dw);
            else change_u((int8_t)up_dw);
        }
        delay_ms(3); // ToDo: remove satic delay!
    }
}

static void change_u (int8_t add) {
    uint16_t task = get_task_u();
    task += add;
    if (task > MaxU) task = U_V(0,2);
    else if (task < U_V(0,2)) task = MaxU;
    set_task_u(task);
}

static void change_ic (int8_t add) {
    uint16_t task = get_task_ic();
    task += add;
    if (task > MaxI) task = I_A(0,2);
    else if (task < I_A(0,2)) task = MaxI;
    set_task_ic(task);
}

static void change_id (int8_t add) {
    uint16_t task = get_task_id();
    task += add;
    if (task > MaxId) task = ID_A(0,2);
    else if (task < ID_A(0,2)) task = MaxId;
    set_task_id(task);
}

static inline void key_power_led (void) {
    if (CsuState != STOP) csu_stop(STOP);
    else csu_start(CHARGE);
    KeyDel = get_fin_time(MS(800));
}

static inline void key_U_up (void) {
    KeyDel = get_fin_time(MS(2));
}

static inline void key_U_dw (void) {
    if (CsuState == CHARGE) {
        if (PWM_U > Step) PWM_U -= Step;
        else PWM_U = 0;
    }
    KeyDel = get_fin_time(MS(2));
}

static inline void key_I_up (void) {
    KeyDel = get_fin_time(MS(2));
}

static inline void key_I_dw (void) {
    if (CsuState == CHARGE) {
        if (PWM_I > Step) PWM_I -= Step;
        else PWM_I = 0;
    }
    KeyDel = get_fin_time(MS(2));
}