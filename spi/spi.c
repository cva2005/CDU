/*
 * Драйвер SPI Master
 */
#include "../sys/system.h"
#include "spi.h"
#include "spi_imp.h"

static unsigned char spi_buf[SPI_BUFFER_SIZE];
static unsigned char wr_ptr, rd_ptr;
static unsigned char wr_len, rd_len;
static unsigned char wr_len, rd_len;
static cs_func_t cs_func; /* функция выбора ведомого */

void spi_init(void)
{
    SET_PIN(SPI_PORT, SCK);
    SET_AS_OUTS(SPI_PORT, SHL(SCK) | SHL(MOSI) | SHL(_SS));
    SPCR = SPI_OFF;
}

/* If SPI Interrupt is enabled then the Transceiver is busy */
bool spi_busy(void)
{
    return ((SPCR & (1 << SPIE)) != 0);
}

void spi_start_io(char *msg, unsigned char wlen,
                  unsigned char rlen, SPI_CNTR const *cntr)
{
    while (spi_busy());
    rd_len = rlen;
    spi_dbprintf("spi_start_io: read: %u, write: %u\r\n", rlen, wlen);
    if (wr_len = wlen) { /* write operation */
        wr_ptr = 1;
        while (wlen--) { /* copy data */
            spi_buf[wlen] = msg[wlen];
            spi_dbprintf("spi_start_io: data[%u]=0x%x\r\n", wlen, msg[wlen]);
        }
    } else if (!rlen) {
        return; /* empty spi task */
    } else {
        wr_ptr = 0;
    }
    rd_ptr = 0;
    cs_func = cntr->cs_func;
    spi_dbprintf("spi_start_io: SPSR = 0x%x, SPCR = 0x%x\r\n",
                 cntr->spsr_val, cntr->spcr_val);
    SPSR = cntr->spsr_val;
    SPCR = cntr->spcr_val;
    cs_func(CS_ON);
    SPDR = spi_buf[0]; /* start spi task */
}

void spi_get_data(char *msg, unsigned char len)
{
    while (spi_busy());
    while (len--) { /* copy data from buffer */
        spi_dbprintf("spi_get_data: data[%u]=0x%x\r\n", len, spi_buf[len]);
        *msg++ = spi_buf[len];
    }
}

#pragma vector=SPI_STC_vect
__interrupt void spi_irq(void)
{
    __enable_interrupt();
    if (wr_ptr < wr_len) { /* write operation */
        SPDR = spi_buf[wr_ptr++];
    } else if (wr_ptr == wr_len) { /* end of write operation */
        if (!rd_len) {
            goto Stop;
        } else {
            wr_ptr++;
            SPDR = SPDR; /* read first byte */
        }
    } else if (rd_ptr < rd_len) { /* read operation */
        spi_buf[rd_ptr++] = SPDR;
        if (rd_ptr == rd_len) goto Stop;
        SPDR = SPDR; /* read next byte */
    } else { /* stop SPI operation */
Stop:
        SPCR &= ~SHL(SPIE);
        cs_func(CS_OFF);
    }
}
