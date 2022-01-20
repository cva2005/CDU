#ifndef SPI_IMP_H
#define SPI_IMP_H
#pragma message	("@(#)spi_imp.h")

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Драйвер SPI Master
 */

/* линии интерфейса SPI */
#define SPI_PORT    B
#define _SS         4
#define SCK         7
#define MOSI        5
#define MISO        6

#define BLEN        4
#define SPI_OFF     0
#define SPI_ON      SHL(SPE) | SHL(MSTR)

#ifdef __cplusplus
}
#endif

#endif /* SPI_IMP_H */
