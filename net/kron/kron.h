#ifndef KRON_H
#define KRON_H
#pragma message	("@(#)kron.h")

/*
 * Драйвер сетевого протокола DCON
 */

#ifdef	__cplusplus
extern "C" {
#endif
void kron_drv (unsigned char ip, unsigned char len);
extern BUS_STATE KronBusState; /* машина сотояния према кадра */
extern unsigned char KronIdleCount; /* счетчик интервалов времени */
#define FRAME_KRON_SYNC     10 /* interval of idle */

#ifdef __cplusplus
}
#endif

#endif /* KRON_H */
