#pragma message	("@(#)key.c")
#include <sys/config.h>
#include "csu/csu.h"
#include "csu/mtd.h"
#include "lcd/lcd.h"
#include "pwm/pwm.h"
#include "key.h"

stime_t KeyDel, KeyPress;
unsigned char Step = 1;

static void key_set (void);
static void key_up (void);
static void key_dw (void);
static void key_power_led (void);
static void key_U_up (void);
static void key_U_dw (void);
static void key_I_up (void);
static void key_I_dw (void);

void check_key (void) {
    uint8_t key, mask;
    if (Cfg.bf1.EXT_Id) mask = 0x08; // no scan K1
    else mask = 0;
    key = KEY_MASK | mask; // scan keys
    if (key != ALL_OFF) { // key press!
        for (uint8_t cnt = 0; cnt < 32; cnt++) { // stable cycle
            if (key != (KEY_MASK | mask)) goto no_press;
        }
        if (!get_time_left(KeyDel)) { //если разрешена обработка кнопок
            //KeyDel = get_fin_time(MS(800));
            if (Cfg.bf1.LCD_ON) {
                if (!get_time_left(KeyPress)) { // клавишу удерживали нажатой
                    KeyPress = get_fin_time(MS(3200));
                    if (Step == 1) Step = 10;
                    else Step = 20;
                }
                if (key==K5) key_power(); // кнопка Старт/Стоп
                if (key==K4) key_set(); // кнопка Set
                if (key==K3) key_up(); // кнопка вверх
                if (key==K2) key_dw(); // кнопка вниз		
            }
            if (Cfg.bf1.LED_ON) {
                if (!get_time_left(KeyPress)) { // клавишу удерживали нажатой
                    KeyPress = get_fin_time(MS(1600));
                    if (Step == 1) Step = 5;
                    else Step = 10;
                }
                if (key == K5) key_power_led(); // кнопка Старт/Стоп
                if (key == K4) key_U_up(); // кнопка Set
                if (key == K3) key_U_dw(); // кнопка вверх
                if (key == K2) key_I_up(); // кнопка вниз	
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
    if (CsuState == STOP && !Error) {
        SelfCtrl = true;
        start_mtd(1);
    } else {
        Stop_CSU(STOP);
        read_mtd();
        lsd_update_set();
    }
    KeyDel = get_fin_time(MS(800));
}

static void key_set (void) {
    if (CsuState == STOP) {
        Cursor_point++;
        if (Cursor_point == PR_NUM) Cursor_point=0;
    } else {
        Cursor_point=0;
        lsd_mode_ch();
    }
    WH2004_inst_wr(Cursor_pos[Cursor_point]);
    KeyDel = get_fin_time(MS(320));
}

static void key_up (void) {
    if (CsuState == STOP) {
        SaveMtd = true;
        switch (Cursor_pos[Cursor_point]) {
        case pr_mode:
            mCnt++;
            read_mtd();
            SaveMtd = false;
            goto set_del_320;
        case pr_I:
            if (SetMode==DISCHARGE) {
                set_Id+=Step;
                if (set_Id>max_set_Id) set_Id=Id_A(0,2);
            } else {
                set_I+=Step;
                if (set_I>max_set_I) set_I=I_A(0,2);
            }
            break;
        case pr_U:
            set_U+=Step;
            if (set_U>max_set_U) set_U=U_V(0,2);
            break;
        case pr_time:
            if (Mtd.fld.end.h < 100) {
                Mtd.fld.end.m += Step;
                if (Mtd.fld.end.m > 59) {
                    Mtd.fld.end.h++;
                    Mtd.fld.end.m = 0;
                }
            }
            KeyDel = get_fin_time(MS(160));
            break;
        case pr_cycle:
            Mtd.fld.Cnt++;
            if (Mtd.fld.Cnt > 10) Mtd.fld.Cnt = 1;
        set_del_320:
            KeyDel = get_fin_time(MS(320));
        }
        lsd_update_set();
    } else {
        if (SetMode == DISCHARGE) {
            set_Id++;
            if (set_Id > max_set_Id) set_Id = Id_A(0,2);
        } else {
            if (I_St) {
                set_I++;
                if (set_I > max_set_I) set_I = I_A(0,2);
            } else {
                set_U++;
                if (set_U > max_set_U) set_U = U_V(0,2);
            }
        }
        delay_ms(3);
    }
}

static void key_dw (void) {
	if (CsuState == STOP) {
        switch (Cursor_pos[Cursor_point]) {
        case pr_mode:
            if (mCnt>0) mCnt--;
            read_mtd();
            KeyDel=20;
            break;
        case pr_I:
            if (SetMode==DISCHARGE) {
                set_Id-=Step;
                if ((set_Id < Id_A(0,2)) || (set_Id > max_set_Id)) set_Id = max_set_Id;
            } else {
                set_I-=Step;
                if ((set_I < I_A(0,2)) || (set_I > max_set_I)) set_I = max_set_I;
            }
            SaveMtd = true;
            break;
        case pr_U:
            set_U-=Step;
            if ((set_U < U_V(0,2)) || (set_U > max_set_U)) set_U = max_set_U;
            SaveMtd = true;
            break;
        case pr_time:
            Mtd.fld.Mm-=Step;
            if (Mtd.fld.Mm>59) {
                if (Mtd.fld.Hm>0) Mtd.fld.Hm--;
                Mtd.fld.Mm=59;
            }
            SaveMtd = true;
            KeyDel=10;
            break;
        case pr_cycle:
            Mtd.fld.Cnt--;
            if (Mtd.fld.Cnt<1) Mtd.fld.Cnt=10;	
            SaveMtd = true;
            KeyDel=20;
        }
		lsd_update_set();
    } else {
		if (SetMode==DISCHARGE) {
			set_Id--;
			if ((set_Id<Id_A(0,2))||(set_Id>max_set_Id)) set_Id=max_set_Id;
			}
		else
			{
			if (I_St)
				{
				set_I--;
				if ((set_I<I_A(0,2))||(set_I>max_set_I)) set_I=max_set_I;
				}
			else
				{
				set_U--;
				if ((set_U<U_V(0,2))||(set_U>max_set_U)) set_U=max_set_U;
				//KeyDel=2;
				}
			}
		delay_ms(3);
		}//*/
}



static void key_power_led (void)
{
	if (CsuState!=0)  //если блок уже запущен
		{
		Stop_CSU(STOP);
		}
	else
		{
		Start_CSU(1);
		}
	KeyDel=50;
}

static void key_U_up (void)
{
	if (CsuState==1)  //если блок запущен
		{
		if (P_wdU<(max_pwd_U-Step)) P_wdU+=Step;
		else P_wdU=max_pwd_U;
		}//if (CsuState==0)
	KeyDel=1;
}


static void key_U_dw (void)
{
	if (CsuState==1)  //если блок запущен
		{
		if (P_wdU>Step) P_wdU-=Step;
		else P_wdU=0;
		}//if (CsuState==0)
	KeyDel=1;
}

static void key_I_up (void)
{
	if (CsuState==1)  //если блок запущен
		{
		if (P_wdI<(max_pwd_I-Step)) P_wdI+=Step;
		else P_wdI=max_pwd_I;
		//if (P_wdI<MAX_CK) P_wdI++;
		}//if (CsuState==0)
	KeyDel=1;
}

static void key_I_dw (void)
{
	if (CsuState==1)  //если блок запущен
		{
		if (P_wdI>Step) P_wdI-=Step;
		else P_wdI=0;
		//if (P_wdI>0) P_wdI--;
		}//if (CsuState==0)
	KeyDel=1;
}//*/