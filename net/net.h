#ifndef NET_H
#define NET_H
#pragma message	("@(#)net.h")

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Драйвер RS485
 */

#include <net/net_imp.h>

/* тип машины сотояния према кадра */
typedef enum {
    BUS_IDLE  = 0,
    BUS_STOP  = 1,
    BUS_START = 2,
    BUS_FULL  = 3
} BUS_STATE;

/* Settable Baud Rate */
typedef enum {
    BAUD_2400   = 0,
    BAUD_4800   = 1,
    BAUD_9600   = 2,
    BAUD_14400  = 3,
    BAUD_19200  = 4,
    BAUD_28800  = 5,
    BAUD_38400  = 6,
    BAUD_57600  = 7,
    BAUD_115200 = 8
} BAUD_RATE;

/* Settable Data Bits */
typedef enum {
    DATABITS_7  = 0,
    DATABITS_8  = 1
} DATA_BITS;

/* Settable Stop bits */
typedef enum {
    STOPBITS_1 = 0, /* One Stop bits */
    STOPBITS_2 = 1  /* Two Stop bits */
} STOP_BITS;

/* Settable Parity bits */
typedef enum {
    PARITY_NONE = 0,
    PARITY_EVEN = 1,
    PARITY_ODD  = 2
} PARITY;

/* Settable Device Address bits */
typedef enum {
    ADDRBITS_8  = 0,
    ADDRBITS_11 = 1
} ADDR_BITS;

/* размер кольцевого буфера приема */
#define RX_BUFF_LEN        32

extern unsigned char *BuffPtr; /* указатель на буфер приема/передачи */
extern unsigned char TxIpBuff; /* указатель данных в буфере передачи */
extern unsigned char BuffLen; /* размер буфера при передаче кадра */
extern unsigned char RsError; /* регистр ошибки сети RS232/RS485 */
extern unsigned char RxBuff[RX_BUFF_LEN]; /* кольцевой буфер приема */
extern signed char RxIpNew; /* указатель хвоста буфера приема */
extern signed char RxIpOld; /* указатель головы буфера приема */


void init_rs (void);
void net_drv (void);
void start_tx (char first, unsigned char *buff);
bool rs_active (void);
void set_active (void);

#include <net/modbus/mbus.h>
#include <net/kron/kron.h>

#ifdef __cplusplus
}
#endif

#endif /* NET_H */
