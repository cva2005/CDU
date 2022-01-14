#ifndef MODBUS_H
#define MODBUS_H
#pragma message	("@(#)mbus.h")

/*
 * Драйвер сетевого протокола MODBUS
 */

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/stime.h>

#define MB_START        0x3a /* символ начала кадра MODBUS ASCII */
#define MB_RTU_RX_MIN   4  /* минимальная длина кадра MODBUS RTU при приеме */

/*
 * Значения таймера тайм-аута режима MODBUS RTU.
 * Квант времени t0.5 (время следования 0.5 символов)
 * выбирается как наибольший общий делитель нацело
 * интервалов протокола (t3.5, t1.5 inter-character time-out).
 * Время следования символа учитывает максимально возможное
 * количество битов на символ при передаче.
 */

#define BITS_PER_CHAR       11
#ifdef RTU_STD_MODE
/* strictly respected MODBUS RTU specification */
#define T05_RTU(baud) (((F_CPU_HZ * BITS_PER_CHAR) / baud) / 2)
#define FRAME_RTU_SYNC  22 /* "режим молчания" активен > 3.5 интервала */
#define FRAME_RTU_ERROR 10 /* "режим молчания" активен > 1.5 интервала */
#else
/*
 * Modbus_over_serial_lineV1.pdf    page 13/44   MODBUS.ORG
 * The implementation of RTU reception driver may imply the management of a lot
 * of interruptions due to the t1.5 and t3.5 timers. With high communication
 * baud rates, this leads to a heavy CPU load. Consequently these two timers
 * must be strictly respected when the baud rate is equal or lower greater
 * than 19200 Bps, fixed values for the 2 timers should be used than 19200 Bps.
 * For baud rates it is recommended to use a value of 750µs for the 
 * inter-character time-out (t1.5) and a value of 1.750ms for inter-frame
 * delay (t3.5).
 */
#define FIX_TO_BAUD   19200
#define T05_RTU(baud) (((F_CPU_HZ * BITS_PER_CHAR) /\
    (baud > FIX_TO_BAUD ? FIX_TO_BAUD : baud)) / 2)
#define FRAME_RTU_SYNC  132 /* "режим молчания" активен > 3.5 интервала */
#define FRAME_RTU_ERROR 60 /* "режим молчания" активен > 1.5 интервала */
#endif

extern void ascii_drv(unsigned char ip, unsigned char len);
extern void rtu_drv(unsigned char ip, unsigned char len);

extern BUS_STATE RtuBusState; /* машина сотояния према кадра RTU */
extern BUS_STATE AsciiBusState; /* машина сотояния према кадра ASCII */
extern unsigned char RtuIdleCount; /* счетчик интервалов времени RTU */
extern unsigned char AsciiIdleCount; /* счетчик интервалов времени ASCII */

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_H */
