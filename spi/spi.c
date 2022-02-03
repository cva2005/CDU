/*
 * Драйвер SPI Master
 */
#pragma message	("@(#)spi.c")
#include <system.h>
#include "spi.h"
#include "spi_imp.h"

static uint8_t spi_buf[BLEN];
static uint8_t wr_ptr, rd_ptr;
static uint8_t wr_len, rd_len;
static cs_func_t cs_func; /* функция выбора ведомого */

void spi_init(void) {
    SET_AS_OUTS(SPI_PORT, SHL(SCK) | SHL(MOSI) | SHL(_SS));
}

void spi_reset (void) {
    SPCR = SPI_OFF;
    CLR_PIN(SPI_PORT, SCK);
}

void spi_start_io (char *msg, uint8_t wlen,
                  uint8_t rlen, cntr_t const *cntr) {
    rd_len = rlen;
    if (wr_len = wlen) { /* write operation */
        wr_ptr = 1;
        while (wlen--) { /* copy data */
            spi_buf[wlen] = *msg++;
         }
    } else if (!rlen) {
        return; /* empty spi task */
    } else {
        wr_ptr = 0;
    }
    rd_ptr = 0;
    cs_func = cntr->cs_func;
    SPSR = cntr->spsr_val;
    SPCR = cntr->spcr_val;
    cs_func(CS_ON);
    SPDR = spi_buf[0]; /* start spi task */
}

void spi_get_data (char *msg, uint8_t first, uint8_t len) {
    while (len--) { /* copy data from buffer */
        *msg++ = spi_buf[len + first];
    }
}

#ifdef SPI_POOL
void spi_pool (void) {
    if (!DATA_RD) return;
#else
#pragma vector=SPI_STC_vect
__interrupt void spi_irq (void) {
#endif
    if (rd_ptr < rd_len) { /* read operation */
        spi_buf[rd_ptr++] = SPDR;
        if (rd_ptr < rd_len) {
            if (wr_ptr == wr_len) {
                SPDR = SPDR; /* read next byte */
                return;
            }
        }
    }
    if (wr_ptr < wr_len) { /* write operation */
        SPDR = spi_buf[wr_ptr++];
    } else { /* end of write operation */
        SPCR &= ~(SHL(SPIE) | SHL(SPE));
        cs_func(CS_OFF);
    }
}
