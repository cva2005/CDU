#ifndef KRON_IMP_H
#define KRON_IMP_H

#pragma message	("@(#)kron_imp.h     1.00    09/09/22 OWEN")

/*
 * Драйвер сетевого протокола KRON
 */

#ifdef	__cplusplus
extern "C" {
#endif

#define KRON_RX_MIN         4 /* минимальная длина кадра */
#define KRON_RX_MAX         250 /* максимальная длина кадра */
#define CHAR_START          0x5A /* символ начала кадра */
#define KRON_BUFF_LEN       KRON_RX_MAX

#ifdef __cplusplus
}
#endif

#endif /* KRON_IMP_H */
