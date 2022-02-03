#ifndef KRON_H
#define KRON_H
#pragma message	("@(#)kron.h")

#ifdef	__cplusplus
extern "C" {
#endif

void kron_drv (uint8_t ip, uint8_t len);
extern BUS_STATE KronBusState; /* машина сотояния према кадра */
extern uint8_t KronIdleCount; /* счетчик интервалов времени */
#define FRAME_KRON_SYNC     10 /* interval of idle */

#ifdef __cplusplus
}
#endif

#endif /* KRON_H */
