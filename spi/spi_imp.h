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
#define _SS         0
#define SCK         1
#define MOSI        2
#define MISO        3

#define SPI_BUFFER_SIZE     4

#define SPI_OFF     0

#ifdef SPI_DEBUG
#define	spi_dbprintf dbprintf
#else
#define	spi_dbprintf(...)
#endif /* SPI_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* SPI_IMP_H */
