#ifndef SPI_H
#define SPI_H
#pragma message	("@(#)spi.h")

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Драйвер SPI Master
 */

typedef enum {
    MSB_FIRST = 0, /* MSB of the data word is transmitted first */
    LSB_FIRST = 1  /* LSB of the data word is transmitted first */
} SPI_DATA_ORDER;

typedef enum {
    IDLE_LOW  = 0, /* clock idle low */
    IDLE_HIGH = 1  /* clock idle high */
} SPI_SCK_IDLE;

typedef enum {
    FIRST_EDGE  = 0, /* data valid on first clock edge */
    SECOND_EDGE = 1  /* data valid on second clock edge */
} SPI_DATA_VALID;

/* Тип аргумента функции выбора кристалла "CS" */
typedef enum {
    CS_OFF = 0,
    CS_ON  = 1
} cs_t;

/* тип функции выбора ведомого */
typedef void (*cs_func_t)(cs_t cs);

/*
 * Драйверы более высокого уровня (часов, ЕЕПРОМ, ADC и т.п.)
 * имеют в ПЗУ заполненную структуру типа
 */
typedef struct {
    uint8_t spcr_val; /* образ регистра SPCR */
    uint8_t spsr_val; /* образ регистра SPSR */
    cs_func_t cs_func; /* функция выбора ведомого */
} cntr_t;

#define SCL_FREQ_SEL(freq) \
    (\
    freq > (F_CPU_HZ / 2)  ? 0 : (\
    freq > (F_CPU_HZ / 4)  ? 0 : (\
    freq > (F_CPU_HZ / 16) ? 1 : (\
    freq > (F_CPU_HZ / 64) ? 2 : 3\
    ))))

#ifdef SPI_POOL
#define IRQ_BIT     0
#else
#define IRQ_BIT     SHL(SPIE)
#endif
/* макроподстановка создания структуры SPI_CNTR в ПЗУ для устройства SPI */
#define MAKE_SPI_CNTR(\
    name, /* имя управляющей структуры */\
    data_order, /* порядок следования битов */\
    sck_freq, /* частота шины SPI, Hz */\
    sck_polar, /* полярность тактового сигнала */\
    sck_phase, /* фаза тактового сигнала */\
    func_cs) /* функция выбора ведомого */\
static const cntr_t name = {\
    (IRQ_BIT | SHL(SPE) | SHL(MSTR) |\
    (SCL_FREQ_SEL(sck_freq) << SPR0) | (data_order << DORD) |\
    (sck_polar << CPOL) | (sck_phase << CPHA)),\
    ((F_CPU_HZ / 2) < sck_freq) ? SHL(SPI2X) : 0,\
    func_cs\
}

void spi_init (void);
void spi_reset (void);
void spi_get_data (char *msg, uint8_t first, uint8_t len);
void spi_start_io (char *msg, uint8_t wlen,
                  uint8_t rlen, cntr_t const *cntr);
#ifdef SPI_POOL
void spi_pool (void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SPI_H */
