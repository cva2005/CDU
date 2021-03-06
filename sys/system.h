#ifndef SYSTEM_H
#define SYSTEM_H
#pragma message	("@(#)sysytem.h")

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef ENABLE_BIT_DEFINITIONS
#define  ENABLE_BIT_DEFINITIONS
#endif /* ENABLE_BIT_DEFINITIONS */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <ina90.h>
#include <ioavr.h>
#include "config.h"
#include "stime.h"

/*
 * Системные константы и определения
 */
typedef enum {
    OFF = 0,
    ON  = 1
} bit_t;

/* битовые операции */
#define SET_BIT(reg,bit) reg |= (1 << bit)
#define CLR_BIT(reg,bit) reg &= ~(1 << bit)
#define IS_BIT_SET(reg,bit) reg & (1 << bit)
#define IS_BIT_CLR(reg,bit) !(reg & (1 << bit))
#define SHL(sh) (1 << sh) /* shift left */

/*
 * Макроподстановка для системного таймера.
 * При использовании битов регистров (E)TIFR, (E)TIMSK
 * необходимо явно указывать имя регистра в коде!
 */
#ifdef SYS_TRM0
#define SYSTRM(P1,P2) P1##0##P2
#elif defined SYS_TRM1
#define SYSTRM(P1,P2) P1##1##P2
#elif defined SYS_TRM2
#define SYSTRM(P1,P2) P1##2##P2
#elif defined SYS_TRM3
#define SYSTRM(P1,P2) P1##3##P2
#else
#error "System timer not defined!"
#endif

/*
 * Макроподстановка для регистров портов ввода/вывода.
 */
#define __set_as_out(port,pin) SET_BIT(DDR##port,pin)
#define SET_AS_OUT(port,pin) __set_as_out(port,pin)
#define __set_as_inp(port,pin) CLR_BIT(DDR##port,pin)
#define SET_AS_INP(port,pin) __set_as_inp(port,pin)
#define __set_pin(port,pin) SET_BIT(PORT##port,pin)
#define SET_PIN(port,pin) __set_pin(port,pin)
#define SET_PUP(port,pin) __set_pin(port,pin)
#define __clr_pin(port,pin) CLR_BIT(PORT##port,pin)
#define CLR_PIN(port,pin) __clr_pin(port,pin)
#define CLR_PUP(port,pin) __clr_pin(port,pin)
#define __is_pin_clr(port,pin) IS_BIT_CLR(PIN##port,pin)
#define IS_PIN_CLR(port,pin) __is_pin_clr(port,pin)
#define __is_pin_set(port,pin) IS_BIT_SET(PIN##port,pin)
#define IS_PIN_SET(port,pin) __is_pin_set(port,pin)
#define __is_lat_clr(port,pin) IS_BIT_CLR(PORT##port,pin)
#define IS_LAT_CLR(port,pin) __is_lat_clr(port,pin)
#define __is_lat_set(port,pin) IS_BIT_SET(PORT##port,pin)
#define IS_LAT_SET(port,pin) __is_lat_set(port,pin)
#define __port_as_out(port) DDR##port = 0xFF
#define PORT_AS_OUT(port) __port_as_out(port)
#define __port_as_inp(port) DDR##port = 0x00
#define PORT_AS_INP(port) __port_as_inp(port)
#define __set_as_outs(port, pins) DDR##port |= pins
#define SET_AS_OUTS(port, pins) __set_as_outs(port, pins)
#define __set_pins(port, pins) PORT##port |= pins
#define SET_PINS(port, pins) __set_pins(port, pins)
#define __set_all(port) PORT##port = 0xFF
#define SET_ALL(port) __set_all(port)
#define __clr_pins(port, pins) PORT##port &= ~pins
#define CLR_PINS(port, pins) __clr_pins(port, pins)
#define __pins(port) PIN##port
#define PINS(port) __pins(port)
#define __lats(port) PORT##port
#define LATS(port) __lats(port)
#define __ddrs(port) DDR##port
#define DDRS(port) __ddrs(port)

/* линия порта джампера "Юстировка" */
#define JS_TSK_PORT     A
#define JS_TSK          6
/* режим "Юстировка" */
#define JS_TASK         IS_PIN_CLR(JS_TSK_PORT, JS_TSK)

/* линия порта джампера "сетевые параметры по умолчанию" */
#define RS_DEF_PORT     A
#define RS_DEF          7
/* сетевые параметры по умолчанию */
#define DEFAULT_CROSS   IS_PIN_CLR(RS_DEF_PORT, RS_DEF)

#if 0
/* линия порта для индикации аварии */
#define ER_LED_PORT     D
#define ER_LED          2
#define	ER_LED_INIT()   SET_AS_OUT(ER_LED_PORT, ER_LED) /* выходная линия */
/* управление индикацией тайм-аута сети */
#define ER_LED_OFF()    SET_PIN(ER_LED_PORT, ER_LED)
#define ER_LED_ON()     CLR_PIN(ER_LED_PORT, ER_LED)
#endif /* #if 0 */

/* значения периода сторожевого таймера (биты WDP регистра WDTCR) */
typedef enum {
    PRD_17MS = 0,
    PRD_34MS,
    PRD_68MS,
    PRD_140MS,
    PRD_270MS,
    PRD_550MS,
    PRD_1100MS,
    PRD_2200MS
} wdt_prd_t;

/* установка периода сторожевого таймера */
#define SET_WDT_PRD(prd)\
{\
    __watchdog_reset();\
    WDTCR = SHL(WDE) | SHL(WDCE);\
    WDTCR = prd | SHL(WDE);\
}

#define D1K     1000
#define D10K    10000
#define D100K   100000
#define D1M     1000000
#define D10M    10000000
#define D100M   100000000

#define SYS_INIT_CODE   0x00
#define MCP_ERROR_CODE  12
#define ERR_WCODE       UINT16_MAX

#ifdef DEBUG
#define	DBGU_INIT(baud) {\
    UCSR0A = SHL(U2X0);\
    UBRR0L = (F_OSC_HZ / 8 / baud - 1);\
    UCSR0B = SHL(TXEN0);\
}
#ifdef PMG_SPACE
#include <pgmspace.h>
#define PGM (char flash *)(int)
#define	dbprintf(...) printf_P(PGM __VA_ARGS__)
#else /* constants in RAM space */
#include <stdio.h>
extern	void dbprintf(const char *fmt, ...);
//#define	dbprintf printf
#endif /* NDEBUG */
#else
#define	DBGU_INIT(baud)
#define	dbprintf(...)
#endif /* DEBUG */
#if BAND_WIDTH_USE
float flt_exp (float out, float inp, float tau, float bandw);
#else
float flt_exp (float out, float inp, float tau);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_H */
