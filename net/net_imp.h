#ifndef NET_IMP_H
#define NET_IMP_H
#pragma message	("@(#)net_imp.h")

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Драйвер RS485
 */

#include <sys/stime.h>

/* макроподстановка для системного USART'а */
#ifdef SYS_UART0
#define UART(P1,P2) P1##0##P2
#elif defined SYS_UART1
#define UART(P1,P2) P1##1##P2
#elif defined SYS_UART
#define UART(P1,P2) P1##P2
#else
#error "System UART not defined!"
#endif

/* линия порта для переключения прием/передача */
#define RS_DIR_PORT     D
#define RS_DIR          2
#define	RS_DIR_INIT()   SET_AS_OUT(RS_DIR_PORT, RS_DIR) /* выходная линия */
/* управления режимом прием/передача RS485 */
#define	RS485_IN()      CLR_PIN(RS_DIR_PORT, RS_DIR) /* на прием */
#define	RS485_OUT()     SET_PIN(RS_DIR_PORT, RS_DIR) /* передачу */
#define IS_RS485_IN()   IS_LAT_CLR(RS_DIR_PORT, RS_DIR) /* акт. прием */
#define IS_RS485_OUT()  IS_LAT_SET(RS_DIR_PORT, RS_DIR) /* акт. передача */
#define	RX_ACTIVE()     IS_RS485_IN()
#define	TX_ACTIVE()     IS_RS485_OUT()

/* Макроподстановка для 8 битного таймера RX */
#define RX_TRM(P1,P2) P1##2##P2
#define RX_OCR_IRQ TIMER2_COMP_vect

/* разрешить прерывание OCIE */
#define SET_RX_OCIE() SET_BIT(TIMSK, RX_TRM(OCIE,))

/* запретить прерывание OCIE */
#define CLR_RX_OCIE() CLR_BIT(TIMSK, RX_TRM(OCIE,))

/* сбросить флаг возможно возникшего прерывания OCF */
#define SET_RX_OCF() CLR_BIT(TIFR, RX_TRM(OCI,))

/* остановить таймер тайм-аута приема кадра */
#define	RX_TMR_OFF()\
{\
    CLR_RX_OCIE(); /* запретить прерывание RX */\
    RX_TRM(TCCR,) = 0; /* остановить таймер RX */\
}

/* полная инициализация приема кадра */
#define START_RX()\
{\
    RX_TMR_OFF(); /* остановить таймер приема кадра */\
    RS485_IN(); /* линию управления RS485 на прием */\
    RxIpOld = RxIpNew; /* указатель хвоста буфера приема */\
    UART(UDR,) = UART(UDR,); /* сбросить флаг возможного прерывания URxC */\
    UART(UCSR,B) |= SHL(UART(RXEN,)) |\
    SHL(UART(RXCIE,)); /* разрешить прием и его прерывание */\
    RtuBusState = /* шина в режиме ожидания */\
    AsciiBusState = /* шина в режиме ожидания */\
    KronBusState = BUS_IDLE; /* шина в режиме ожидания */\
}

#define STOP_RX()\
{\
    UART(UCSR,B) &= ~(SHL(UART(RXEN,)) | SHL(UART(RXCIE,)));\
}

/*
 * The Baud Rate Generator.
 * Equations for Calculating Baud Rate Register Setting.
 *
 */
/*
 * Asynchronous Normal mode (U2Xn = 0):
 * UBRRn = (Fosc / (16 * BAUD)) - 1 */
#ifndef UART_DS_MODE
#define BRG_SET(baud)\
{\
    UART(UBRR,L) = F_OSC_HZ / 16 / baud - 1;\
    UART(UBRR,H) = 0;\
}
#else
/*
 * Asynchronous Double Speed mode (U2Xn = 1):
 * UBRRn = (Fosc / (8 * BAUD)) - 1 */
#define BRG_SET(baud)\
{\
    SET_BIT(UART(UCSR,A), UART(U2X,));\
    UART(UBRR,L) = F_OSC_HZ / 8 / baud - 1;\
    UART(UBRR,H) = 0;\
}
#endif

/* Режимы 8-ти битного таймера тайм-аута RS485. */
/* CTC Mode, No prescaling */
#define RX_TO_MODE     SHL(RX_TRM(CS,1))
    
/* запустить таймер тайм-аута приема кадра */
#define	RX_TMR_ON()\
{\
    if (RtuIdleCount >= FRAME_RTU_ERROR) RtuBusState = BUS_IDLE;\
    RsActiveCount = 0;\
    RtuIdleCount = 0; /* очистить счетчик интервалов MODBUS RTU */\
    AsciiIdleCount = 0; /* очистить счетчик интервалов ASCII */\
    RX_TRM(TCNT,) = 0; /* очистить счетный регистр таймера */\
    RX_TRM(TCCR,) = RX_TO_MODE; /* установить режим */\
    SET_RX_OCIE(); /* разрешить прерывание таймера приема */\
}

/* значение регистра сравнения 8-и битного таймера приема */
#define  T_VAL_RTU  0xFF
/*
 * Значения таймера тайм-аута режима MODBUS ASCII/KRON:
 * MODBUS ASCII/KRON - время следования символов в кадре < 1 сек,
 */
#define FRAME_ASCII_ERROR   0xFF
#define NOT_ACTIVE          2000
/* Установка скорости UART и регистра сравнения таймера тайм-аута */
#define SET_BAUD(baud) {BRG_SET(baud); RX_TRM(OCR,) = T_VAL_RTU;}

/* Тип функций сетевого драйвера верхнего уровня */
typedef void NET_FUNC(unsigned char ip, unsigned char len);

#ifdef NET_DEBUG
#define	net_dbprintf dbprintf
#else
#define	net_dbprintf(...)
#endif /* NET_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* NET_IMP_H */
