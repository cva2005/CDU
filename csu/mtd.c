#pragma message	("@(#)mtd.c")
#include <sys/config.h>
#include "lcd/lcd.h"
#include "spi/adc/ads1118.h"
#include "net/net.h"
#include "pwm/pwm.h"
#include "csu.h"
#include "mtd.h"

stg_t Stg; mtd_t Mtd; fin_t Fin;
uint8_t mCnt = 0, sCnt = 0, cCnt = 0;
uint8_t fCnt;
uint8_t StgNum[MTD_N] = {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint16_t msNum = 0;
bool SaveMtd = false;
uint16_t TaskI, TaskId, TaskU, TaskUmemC, TaskUmemD;
uint16_t MaxU, MaxI, MaxId;
hms_t Tm = {0,0,0}, Ts;
stime_t PulseStep; //время импульса заряд/разряд при импульсном режиме
stime_t dUtime; // Время когда не увеличивается U при заряде щелочного АКБ

static bool end_time (hms_t *tm);

/* проверка условий окончания этапа */
bool fin_cond(void) {
    uint16_t adc_mu = get_adc_res(ADC_MU);
    if (Stg.fld.stop_flag.I) { // признак окончания по току
        if (Stg.fld.type == DISCHARGE) {
            if (get_adc_res(ADC_DI) <= Fin.I) return true; //при разряде проверить что ток стал меньше минимального допустимого
        } else {
            if (CsuState == CHARGE) {
                if (get_adc_res(ADC_MI) <= Fin.I) return true;
            } //при заряде проверить что ток стал меньше минимально допустимого
        }	
        //если условия выполняются то не сбрасывать счётчик антидребезга
    }
    if (Stg.fld.stop_flag.U) { //если есть признак окончания по напряженияю
        if ((Stg.fld.type==DISCHARGE)||(Stg.fld.type==PAUSE)) {
            if (adc_mu <= Fin.U) return true; //при разряде проверить что напряжение меньше допустимого
        } else {
            if (CsuState == CHARGE) {
                if (adc_mu >= Fin.U) return true;
            } //при заряде проверить что напряжение больше допустимого
        }
        //если условия выполняются то не сбрасывать счётчик антидребезга
    }
    if (Stg.fld.stop_flag.T) { //если установлен флаг окончания по времени
        if (end_time(&Stg.fld.end)) { // время этапа вышло
            fCnt = 0;	// уст. флаг окончания метода (сбросить счётчик антидребезка в 0)
            return true;	
        }
    }
    if (Stg.fld.stop_flag.dU) { //проверить условия падения напряжения
        if (Fin.max_U < adc_mu) { //если максимальное зафиксированое напряжение меньше текущего
            Fin.max_U = adc_mu; //запомнить текущее напряжение как максимальное
            dUtime = get_fin_time(DU_TIME);
         } else {
            if ((Fin.max_U - adc_mu) >= Fin.dU) return true;  //проверить разница между текущим и максимальным больше дельты?
        }
        if (!get_time_left(dUtime)) return true;
    }
    fCnt = 30;
    return false;
}

/* проверка текущего сосотяния этапа */
void stg_status (void) {
    if (Stg.fld.type == PULSE) { // импульсный режим
        if (!get_time_left(PulseStep)) { // истекло время импульса
            if (CsuState == DISCHARGE) { // сменить на заряд
                TaskU = TaskUmemC; // напряжение заряда
                csu_start(CHARGE);
                PulseStep = get_fin_time(SEC(Stg.fld.T_ch));
            } else {
                TaskU=TaskUmemD;
                csu_start(DISCHARGE);
                PulseStep = get_fin_time(SEC(Stg.fld.T_dch));
            }
        }	
    }
    fin_cond(); //проверить условие окончания этапа (если да то fCnt не будет перезагружен)
    if (fCnt == 0) { //условия окончания выполняются и вышло время антидребезга?
        if (sCnt + 1 < Mtd.fld.NStg) { //в методе есть информация о следующем этапе?
            sCnt++; // переключиться на следующий этап
            read_stg(sCnt); // прочитать следующий этап
            calc_stg();
            switch (Stg.fld.type) {
            case DISCHARGE:
                if (PwmStatus != DISCHARGE)
                    csu_start(DISCHARGE);
                break;
            case CHARGE:
            case PULSE:
                if (PwmStatus != CHARGE)
                    csu_start(CHARGE);
                 break;
            case PAUSE:
                if (CsuState != PAUSE)
                    csu_start(PAUSE);
           }
        } else { // следующего этапа нет
            if ((cCnt<Mtd.fld.Cnt)||(cCnt>9)) { //есть повотрные циклы?
                if (cCnt<10) cCnt++;
                start_mtd(0);   //если есть, то запустить метод заново
            } else { //если нет не выполненых циклов
                stop_mtd(); //остановить метод
            }
        }
    }
    if (Mtd.fld.end.h < INF_TIME) { // не бесконечный метод
        if (end_time(&Mtd.fld.end)) {
            stop_mtd(); //отсановить метод
        }	
    }
}

/* расчёт параметров метода */
void calc_mtd (void) {
    SetMode = Stg.fld.type;
    TaskU = (uint16_t)U_ADC(Mtd.fld.Um);
    TaskId = (uint16_t)ID_ADC(Mtd.fld.Im);
    TaskI = (uint16_t)I_ADC(Mtd.fld.Im);
    /* Если в методе больше 10 циклов, то ограничить их 10-ю */
    if (Mtd.fld.Cnt > 10) Mtd.fld.Cnt = 10;
}

/* расчёт параметров этапа */
void calc_stg(void) {
    TaskI = (uint16_t)I_M(Mtd.fld.Im, Stg.fld.I_ch);
    if (TaskI > MaxI) TaskI = MaxI;
    TaskId = (uint16_t)ID_M(Mtd.fld.Im, Stg.fld.I_dch);
    if (TaskId > MaxId) TaskId = MaxId;
    TaskUmemC = (uint16_t)U_M(Mtd.fld.Um, Stg.fld.U_ch);
    if (TaskUmemC > MaxU) TaskUmemC = MaxU;
    TaskUmemD = (uint16_t)U_M(Mtd.fld.Um, Stg.fld.U_dch);
    if (TaskUmemD > MaxU) TaskUmemD = MaxU;
    if (Stg.fld.type == DISCHARGE) TaskU = TaskUmemD;
    else TaskU = TaskUmemC;
    if (Stg.fld.stop_flag.U)
        Fin.U = (uint16_t)U_M(Mtd.fld.Um, Stg.fld.end_U);
    if (Stg.fld.stop_flag.dU)
        Fin.dU = (uint16_t)U_M(Mtd.fld.Um, Stg.fld.end_dU);
    if (Stg.fld.stop_flag.I) {
        if (Stg.fld.type==DISCHARGE)
            Fin.I = (uint16_t)ID_M(Mtd.fld.Im, Stg.fld.end_I);
        else
            Fin.I = (uint16_t)I_M(Mtd.fld.Im, Stg.fld.end_I);
	}
    if (Stg.fld.type==STOP) Stg.fld.type = PAUSE;
    Fin.max_U=0;
    PulseStep = get_fin_time(SEC(Stg.fld.T_ch)); // время импульса заряда в имп. режиме
    memset(&Ts, 0, sizeof(Ts));
    fCnt = INF_TIME; //установить паузу в определении условий окончания
}

void read_mtd (void) {
    uint8_t i, n = mCnt;
    for (i = 0; i < mCnt; i++) {
        n += StgNum[i];
    }
    if (eeread_mtd(n, &Mtd) == false) {
        if (mCnt != MTD_DEF) mCnt = n = 0;
        else n = 1;
        //если не причтался не заводской метод, то установить на первый заводской метод
        if (eeread_mtd(n, &Mtd) == false)
            create_mtd(n); //если не прочитался заводской метод, то создать его
    }
    sCnt = 0;
    read_stg(0);
    calc_mtd();
    cCnt = 0; //установить первый цикл
}

void read_stg (uint8_t num) {
    uint8_t i, n = mCnt;
    for (i = 0; i < mCnt; i++) {
        n += StgNum[i];
    }
    n += num;
    if (eeread_stg(n, &Stg) == false) Error = ERR_STG;
}

void start_mtd (uint8_t num) {
    uint32_t s, error_calc;
    memset(&Tm, 0, sizeof(Tm));
	if (num) {
		//рсчитать значения в вольтах
		s = TaskU * Cfg.K_U;
		error_calc = s % 1000000UL;
		if ((error_calc / 100000UL) > 4) s += 1000000UL; //убрать погрешность
		Mtd.fld.Um = (uint16_t)((s - error_calc) / 100000UL);
		//расчитать ток в амперах
		if (Stg.fld.type == DISCHARGE) s = TaskId * Cfg.K_Id;
		else s = TaskI * Cfg.K_I;
		error_calc = s % 1000000UL;
		if ((error_calc / 100000UL) > 4) s += 1000000UL; //убрать погрешность
		Mtd.fld.Im = (uint16_t)((s - error_calc) / 100000UL);
		if (SaveMtd) {
            save_alg(mCnt, &Mtd);
			SaveMtd = false;	
		}			
		cCnt = 1; // set first cycle
	}
	sCnt = 0; // set first stage
	read_stg(sCnt);
	calc_stg();
	if (Stg.fld.type == DISCHARGE) {
        csu_start(DISCHARGE);
	} else {
		if (Stg.fld.type==PAUSE) csu_start(PAUSE);	
		else csu_start(CHARGE);	
	}	
}

void stop_mtd (void) {
    lcd_update_work();
    csu_stop(STOP);
    if (Cfg.mode.lcd) {
        CsuState = CHARGE;
        lcd_stop_msg();
	}
}

void create_mtd (uint8_t num) {
    if (num < 2) {
        memset(Mtd.fld.name, ' ', sizeof(Mtd.fld.name));
        Mtd.fld.data_type = MTD_ID;
        Mtd.fld.end.h = 10;
        Mtd.fld.Cnt =
        Mtd.fld.NStg = 1;
        Stg.fld.data_type = STG_ID;
        Mtd.fld.end.m =
        Mtd.fld.end.s =
        Stg.fld.stop_flag.T =
        Stg.fld.stop_flag.I =
        Stg.fld.stop_flag.U =
        Stg.fld.stop_flag.dU =
        Stg.fld.T_ch =
        Stg.fld.T_dch =
        Stg.fld.end_I =
        Stg.fld.end_U =
        Stg.fld.end_dU =
        Stg.fld.end_Temp =
        Stg.fld.end_C =
        Stg.fld.end.m =
        Stg.fld.end.s = 0;
        Mtd.fld.Im =
        Stg.fld.end.h = INF_TIME;
    } else return;
    if (num == 0) {
        Mtd.fld.name[0] = Z_rus;
        Mtd.fld.name[1] = 'a';
        Mtd.fld.name[2] = 'p';
        Mtd.fld.name[3] = ya_rus;
        Mtd.fld.name[4] = d_rus;
        Mtd.fld.Um = 1440;
        Stg.fld.type = CHARGE;
        Stg.fld.I_ch =
        Stg.fld.U_ch = 10000;
        Stg.fld.I_dch =
        Stg.fld.U_dch = 0;
	} else if (num == 1) {
        Mtd.fld.name[0] = 'P';
        Mtd.fld.name[1] = 'a';
        Mtd.fld.name[2] = z_rus;
        Mtd.fld.name[3] = 'p';
        Mtd.fld.name[4] = ya_rus;
        Mtd.fld.name[5] = d_rus;
        Mtd.fld.Um = 1080;
        Stg.fld.type = DISCHARGE;
        Stg.fld.I_ch =
        Stg.fld.U_ch = 0;
        Stg.fld.I_dch =
        Stg.fld.U_dch = 10000;
	}
    save_alg(num, &Mtd);
    save_alg(num + 1, &Stg);
}

uint8_t find_free (void) {
    uint8_t i, s, n = mCnt;
    for (i = 0; i < mCnt/*MS_N*/; i++) {
        n += StgNum[i];
    }
    mtd_t mtd;
    i = mCnt;
    while (n < MS_N) {
        if (eeread_mtd(n, &mtd) == false) break;
        i++;
        s = mtd.fld.NStg;
        StgNum[i] = s;
        n++;
        n += s;
    }
    return n;
}

void delete_all_mtd (void) {
    eeclr_alg();
    for (uint8_t i = 0; i < MS_N; i++) {
        StgNum[i] = 0;
    }
    mCnt = sCnt = cCnt = 0;
}

/* ToDo: compare as uint32_t */
static bool end_time (hms_t *tm) {
    if (Ts.h > tm->h) return true;
    if (Ts.h == tm->h) {
        if (Ts.m > tm->m) return true;
        if (Ts.m == tm->m) {
            if (Ts.s >= tm->s) return true;
        }
    }
    return false;
}