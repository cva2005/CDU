#pragma message	("@(#)kron.h     1.00    09/09/22 OWEN")

#ifndef KRON_H
#define KRON_H

/*
 * Драйвер сетевого протокола DCON
 */

#ifdef	__cplusplus
extern "C" {
#endif
void kron_drv(unsigned char ip, unsigned char len);
extern BUS_STATE KronBusState; /* машина сотояния према кадра */
extern unsigned char KronIdleCount; /* счетчик интервалов времени */
#define FRAME_KRON_ERROR 10 /* "режим молчания" активен > 1.5 интервала */

#ifdef __cplusplus
}
#endif

#endif /* KRON_H */
