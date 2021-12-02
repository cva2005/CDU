#ifndef NET_IMP_H
#define NET_IMP_H

#pragma message	("@(#)net_imp.h     1.00    09/08/12 OWEN")

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

#define BRGL_REG UART(UBRR,L)

/*
 * Макроподстановка для 8 битного таймера RTU.
 * При использовании битов регистров (E)TIFR, (E)TIMSK
 * необходимо явно указывать имя регистра в коде!
 */
#define RTUTRM(P1) P1##2
#define RTU_OCR_IRQ TIMER2_COMP_vect
#define RTU_TIMSK ETIMSK
#define RTU_TIFR ETIFR

/* линия порта для переключения прием/передача */
#define RS_DIR_PORT     D
#define RS_DIR          2
#define	RS_DIR_INIT()   SET_AS_OUT(RS_DIR_PORT, RS_DIR) /* выходная линия */
/* управления режимом прием/передача RS485 */
#define	RS485_IN()      CLR_PIN(RS_DIR_PORT, RS_DIR) /* на прием */
#define	RS485_OUT()     SET_PIN(RS_DIR_PORT, RS_DIR) /* передачу */
#define IS_RS485_IN()   IS_LAT_CLR(RS_DIR_PORT, RS_DIR) /* акт. прием */
#define IS_RS485_OUT()  IS_LAT_SET(RS_DIR_PORT, RS_DIR) /* акт. передача */

/* генерация имени регистра сравнения 16-ти битного таймера OCRuX */
#define OCRnX(X)  RSTRM(OCR,X)

/* генерация имени вектора прерывания 16-ти битного таймера OCRuX */
#define RS_OCRnX_IRQ(X)    RS_OCR_IRQ(X)

/* разрешить прерывание OCIEnX */
#define SET_RS_OCIE(X) SET_BIT(RS_TIMSK, RSTRM(OCIE,X))
#define SET_RTU_OCIE() SET_BIT(RTU_TIMSK, RTUTRM(OCIE,A))

/* запретить прерывание OCIEnX */
#define CLR_RS_OCIE(X) CLR_BIT(RS_TIMSK, RSTRM(OCIE,X))
#define CLR_RTU_OCIE() CLR_BIT(RTU_TIMSK, RTUTRM(OCIE,A))

/* сбросить флаг возможно возникшего прерывания OCFnX */
#define SET_RS_OCF(X) CLR_BIT(RS_TIFR, RSTRM(OCIE,X))
#define SET_RTU_OCF() CLR_BIT(RTU_TIFR, RTUTRM(OCIE,A))

/* остановить таймер тайм-аута приема кадра RTU */
#define	RTU_TMR_OFF()\
{\
    CLR_RTU_OCIE(); /* запретить прерывание RTU */\
    RTUTRM(TCCR,B) = 0; /* остановить таймер RTU */\
    RtuIdleCount = 0; /* очистить счетчик интервалов MODBUS RTU */\
}

/* остановить таймер тайм-аута приема кадра ASCII */
#define	ASCII_TMR_OFF()\
{\
    AsciiIdleCount = 0; /* очистить счетчик интервалов ASCII */\
    RSTRM(TCCR,B) = 0; /* остановить таймер ASCII */\
    CLR_RS_OCIE(ASCII); /* запретить прерывание ASCII */\
}

/* активизация режима передачи кадра */
#define	FRAME_TX_ON()\
{\
    SET_RS_OCIE(TXDEL); /* разрешить прер. от таймера задержки предачи */\
}

/* состояние передачи кадра */
#define	TX_ACTIVE() IS_RS485_OUT()

/* полная инициализация приема кадра */
#define START_RX()\
{\
    ASCII_TMR_OFF(); /* остановить таймер ASCII */\
    RTU_TMR_OFF(); /* остановить таймер RTU */\
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
#define BRG_SET(baud) BRGL_REG = F_OSC_HZ / 16 / baud - 1
#else
/*
 * Asynchronous Double Speed mode (U2Xn = 1):
 * UBRRn = (Fosc / (8 * BAUD)) - 1 */
#define BRG_SET(baud)\
{\
    SET_BIT(UART(UCSR,A), UART(U2X,));\
    BRGL_REG = F_OSC_HZ / 8 / baud - 1;\
}
#endif

/*
 * Значения таймера тайм-аута режима MODBUS ASCII/KRON:
 * MODBUS ASCII/KRON - время следования символов в кадре < 1 сек,
 * OWEN - время следования символов в кадре < 50 мс.
 * Наименьшее кратное - 50 мс.
 */
#define TO_ASCII_MS     50
#define TO_ASCII_PRD    TO_ASCII_MS / MILLISEC
#define TO_ASCII_PS     8 /* предделитель таймера */
#define TO_ASCII_VAL    (((F_CPU_HZ / TO_ASCII_PS) * TO_ASCII_PRD) - 1)
#define FRAME_ASCII_ERROR   (MILLISEC / TO_ASCII_MS)

/*
 * Макроподстановка для 16-ти битного таймера тайм-аута MODBUS ASCII/OWEN/DCON
 */
#if (TO_ASCII_PS == 1)
#define TO_CS_VAL 1
#elif (TO_ASCII_PS == 8)
#define TO_CS_VAL 2
#elif (TO_ASCII_PS == 64)
#define TO_CS_VAL 3
#elif (TO_ASCII_PS == 256)
#define TO_CS_VAL 4
#elif (TO_ASCII_PS == 1024)
#define TO_CS_VAL 5
#else
#error "ASCII timer prescaler value not correct!!"
#endif

/* Режимы 16-ти битного таймера тайм-аута RS485. */
/* MODBUS RTU режим, (CTC Mode, No prescaling) */
#define RTU_TO_MODE     (SHL(RTUTRM(CS,0)) | SHL(RTUTRM(WGM,2)))
/* ASCII режим,(CTC Mode, + prescaler) */
#define ASCII_TO_MODE   (SHL(RSTRM(WGM,2)) | (TO_CS_VAL << RSTRM(CS,0)))
    
/* запустить таймер тайм-аута приема кадра RTU */
#define	RTU_TMR_ON()\
{\
    if (RtuIdleCount >= FRAME_RTU_ERROR) RtuBusState = BUS_IDLE;\
    RtuIdleCount = 0; /* очистить счетчик интервалов MODBUS RTU */\
    RTUTRM(TCNT,) = 0; /* очистить счетный регистр таймера RTU */\
    RTUTRM(TCCR,B) = RTU_TO_MODE; /* установить режим RTU */\
    SET_RTU_OCIE(); /* разрешить прерывание RTU */\
}

/* запустить таймеры тайм-аута приема кадра ASCII */
#define	ASCII_TMR_ON()\
{\
    AsciiIdleCount = 0; /* очистить счетчик интервалов ASCII */\
    RSTRM(TCNT,) = 0; /* очистить счетный регистр таймера ASCII */\
    SET_BIT(SFIOR, PSR321); /* сбросить предделитель таймера ASCII */\
    RSTRM(TCCR,B) = ASCII_TO_MODE; /* установить режим ASCII */\
    SET_RS_OCIE(ASCII); /* разрешить прерывание ASCII */\
}

/* регистр сравнения 16-ти битного таймера OCRnX для режима MODBUS RTU */
#if TIMER_3_USE
#define RTU_TREG    RTUTRM(OCR,A)
#else
#define RTU_TREG    RTUTRM(OCR)
#endif

/* регистр сравнения 16-ти битного таймера OCRnX для режима ASCII */
#define ASCII_TREG  OCRnX(ASCII)

/* значение регистра сравнения 8-и битного таймера OCRnX для режима RTU */
#define  T_05_RTU  0xFF

#define SET_BAUD(baud) {BRG_SET(baud); RTU_TREG = T_05_RTU;}

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
