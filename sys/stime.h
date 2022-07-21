#ifdef	__cplusplus
extern "C" {
#endif

#ifndef STIME_H
#define STIME_H
#pragma message	("@(#)stime.h")

/*
 * Системное время
 */

#include "system.h"

#define MILLISEC    1000UL
#define MICROSEC    1000000UL
#define NANOSEC     1000000000UL

/* Частота осцилятора, Hz */
#ifdef INT_FREQ
#define F_OSC_HZ    8000000UL
#else
#define F_OSC_HZ    16000000UL
#endif /* INT_FREQ */

/* Тактовая частота CPU, Hz */
#define F_CPU_HZ    F_OSC_HZ

/* Период системного таймера, мс */
#define SYS_PRD_MS  10
/* Частота следования прерываний системного таймера, Hz */
#define F_SYS_HZ    100

#define SYS_TMR_PRE 1024

/*
 * Макроподстановка для 8-битного системного таймера.
 */
#ifdef SYS_TRM0
#define SYSTRM(P1,P2) P1##0##P2
#if (SYS_TMR_PRE == 1)
#define CS_VAL 1
#elif (SYS_TMR_PRE == 8)
#define CS_VAL 2
#elif (SYS_TMR_PRE == 64)
#define CS_VAL 3
#elif (SYS_TMR_PRE == 256)
#define CS_VAL 4
#elif (SYS_TMR_PRE == 1024)
#define CS_VAL 5
#else
#error "System timer prescaler value not correct!!"
#endif
#elif defined SYS_TRM2
#define SYSTRM(P1,P2) P1##2##P2
#if (SYS_TMR_PRE == 1)
#define CS_VAL 1
#elif (SYS_TMR_PRE == 8)
#define CS_VAL 2
#elif (SYS_TMR_PRE == 64)
#define CS_VAL 3
#elif (SYS_TMR_PRE == 256)
#define CS_VAL 4
#elif (SYS_TMR_PRE == 1024)
#define CS_VAL 5
#else
#error "System timer prescaler value not correct!!"
#endif
#else
#error "System timer not defined!"
#endif

typedef struct {
	uint32_t del;
	uint32_t run;
} stime_t;

stime_t get_fin_time(uint32_t delay);
uint32_t get_time_left(stime_t stime);
uint32_t get_interval(uint32_t run);

/* Период системного таймера, CPU Cycles */
#define SYS_PRD (((F_CPU_HZ / SYS_TMR_PRE) / F_SYS_HZ) - 1)

#define MS(x)   x > SYS_PRD_MS ? ((x + (SYS_PRD_MS / 2)) / SYS_PRD_MS) : 1
#define SEC(x)  x * (1000 / SYS_PRD_MS)

/* настройка и запуск системного таймера */
#define SYS_TMR_ON()\
{\
    SYSTRM(TCNT,) = 0; /* установить счетный регистр */\
    SYSTRM(OCR,) = SYS_PRD; /* установить регистр сравнения/сброса */\
    /* запустить таймер (СTС Mode, prescaling) */\
    SYSTRM(TCCR,) = SHL(SYSTRM(WGM,1)) | CS_VAL << SYSTRM(CS,0);\
    /* разрешить прерывание OCIEnA */\
    SET_BIT(TIMSK, SYSTRM(OCIE,));\
}

extern uint32_t stime; /* системное время, 100 HZ */

#define get_stime(cur_time)\
{\
    /* запретить прерывание OCn */\
    CLR_BIT(TIMSK, SYSTRM(OCIE,));\
    cur_time = stime;\
    /* разрешить прерывание OCn */\
    SET_BIT(TIMSK, SYSTRM(OCIE,));\
}

#define set_stime(new_time)\
{\
    /* запретить прерывание OCn */\
    CLR_BIT(TIMSK, SYSTRM(OCIE,));\
    stime = new_time;\
    /* разрешить прерывание OCn */\
    SET_BIT(TIMSK, SYSTRM(OCIE,));\
}

#define STIME_MAX   UINT32_MAX

#define stime_diff(t_old, t_new) (uint32_t)((t_new < t_old) ?\
    (((uint32_t)STIME_MAX - t_old) + t_new) : (t_new - t_old))

bool run_time(uint32_t start, uint32_t delta);

#define delay_ns() {asm("nop"); asm("nop"); asm("nop"); asm("nop");}
/*
 * Задержка с микросекундным интервалом.
 * Зависит от количества операций в ассемблерном цикле!
 */
#define ASM_CYCLE_LEN   6
#define delay_us(t_us)\
{\
    uint16_t i = ((t_us * F_CPU_HZ) / MICROSEC) / ASM_CYCLE_LEN;\
    while (i--);\
}

#define delay_ms(t_ms) \
{\
    uint16_t ms = t_ms;\
    while (ms--); {\
        uint16_t us = 1000;\
        while (us--);\
    }\
}

#ifdef __cplusplus
}
#endif

#endif /* STIME_H */
