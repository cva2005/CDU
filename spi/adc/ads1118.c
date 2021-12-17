#pragma message	("@(#)ads1118.c")
#include <system.h>
#include "spi/spi.h"
#include "ads1118.h"
#include "ads1118_imp.h"

static void adc_cs(cs_t cs);

/*
 * Создание структуры - описателя SPI ADC ADS1118.
 * Configure microcontroller SPI interface to SPI
 * mode 1 (CPOL = 0, CPHA = 1).
 */
MAKE_SPI_CNTR(
    adc_cntr, /* имя управляющей структуры */
    MSB_FIRST, /* порядок следования битов */
    SCK_FREQ, /* частота шины SPI, Hz */
    IDLE_LOW, /* полярность тактовых импульсов SPI */
    SECOND_EDGE, /* фаза тактовых импульсов SPI */
    adc_cs); /* функция выбора ведомого */

void adc_init(void)
{
    CS_OFF();
    SET_AS_OUT(CS_PORT, CS_PIN);
}

static void adc_cs(cs_t cs)
{
    ADC_SEL(cs);
}

/*
 * Восстановление связи при нарушении обмена.
 * In situations where the interface sequence is lost, a write operation
 * of at least 32 serial clock cycles with DIN high returns the ADC to
 * this default state by resetting the entire part.
 * This ensures that the interface can be reset to a known state if
 * the interface gets lost due to a software error or some glitch in
 * the system. Reset returns the interface to the state in which it is
 * expecting a write to the communications register. This operation
 * resets the contents of all registers to their power-on values.
 * Following a reset, the user should allow a period of 500 µs
 * before addressing the serial interface.
 */
void adc_reset(void)
{
    char data[] = {0xff, 0xff, 0xff, 0xff};

    spi_start_io(data, sizeof(data), 0, &adc_cntr);
}

/* Расчет Ку усилителя ADC */
unsigned char adc_amp_gain(unsigned short inp, unsigned short ref)
{
    inp += inp / 4; /* +25% for efficient digital filtering */
    if (inp > ref / 2) return 1;
    if (inp > ref / 4) return 2;
    if (inp > ref / 8) return 4;
    if (inp > ref / 16) return 8;
    if (inp > ref / 32) return 16;
    if (inp > ref / 64) return 32;
    if (inp > ref / 128) return 64;
    return 128;
}

/* Чтение данных регистра ADC */
void get_adc_reg(reg_t reg)
{
    COM_REG_T com;

    *((char *)&com) = 0;
    com.rd_wr = READ_REG;
    com.reg_addr = reg;
    spi_start_io((char *)&com, sizeof(COM_REG_T),
                 (reg == STATUS_REG) || (reg == IO_REG) ?
                  sizeof(char) : sizeof(short), &adc_cntr);
}

/* Запись регистра ADC */
void set_adc_reg(REG_TYPE reg, unsigned short val)
{
    WR_BUFFER msg;
    unsigned char len;

    *((char *)&msg.com) = 0;
    msg.com.reg_addr = reg;
    if (reg == IO_REG) {
        len = sizeof(msg) - 1;
        msg.data[0] = (char)val;
    } else {
        len = sizeof(msg);
        msg.data[0] = val >> 8;
        msg.data[1] = (char)val;
    }
    spi_start_io((char *)&msg, len, 0, &adc_cntr);
}

/* Проверка записанного значения регистра ADC */
bool vrf_adc_reg(REG_TYPE reg, char *val)
{
    char buf[2];

    spi_get_data(buf, sizeof(short));
    if (buf[0] == val[0]) {
        if ((reg == IO_REG) ||
            (buf[1] == val[1]))
            return true;
    }
    return false;
}

bool adc_compl(void)
{
    bool compl;

    if (spi_busy()) return false;
    ADC_CS_ON();
    compl = IS_PIN_CLR(ADC_RDY_PORT, ADC_RDY_PIN);
    ADC_CS_OFF();
    return compl;
}

#if 0
ADS1118_type ADC_cfg_wr, ADC_cfg_rd;
ADC_Type  ADC_ADS1118[ADC_CH];
unsigned int ADC_O[ADC_CH];
unsigned char ADS1118_St[ADC_CH];
unsigned char ADS1118_chanal=0;
unsigned char ADC_wait=200;

void SPI_MasterInit(void)
{
//	Set MOSI and SCK output, all others input
//	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK);
    /* Enable SPI, Master, set clock rate fck/4 */
    SPCR = (1 << SPE) | (0 << DORD) | (1 << MSTR)
        | (0 << CPOL) | (1 << CPHA) | (0 << SPR1) | (0 << SPR0);
}

unsigned char SPI_MasterTransmit(unsigned char cData) {
    SPDR = cData;// Start transmission
    while (!(SPSR & (1 << SPIF))); // Wait for transmission complete 
    return (SPDR);
}

void Init_ADS1118(void) {
    SPI_MasterInit();
    ADC_cfg_wr.bit.OS=1;//0 : No effect 1 : Begin a single conversion (when in power-down mode)
    ADC_cfg_wr.bit.MUX_POL=1; //0:differential inputs 1: single-ended inputs
    ADC_cfg_wr.bit.MUX=3; //channel: 0;1;2;3
    ADC_cfg_wr.bit.PGA=2; //0:±6.144V; 1:±4.096V; 2:±2.048V; 3:±1.024V; 4:±0.512V; 5:±0.256V;
    ADC_cfg_wr.bit.MODE=1; //0 : Continuous conversion mode  1 : Power-down single-shot mode
    ADC_cfg_wr.bit.DR=7; //0:8SPS; 1:16SPS; 2:32SPS; 3:64SPS; 4:128SPS; 5:250SPS; 6:475SPS; 7:860SPS;
    ADC_cfg_wr.bit.TS_MODE=0; //0 : ADC mode  1 : Temperature sensor mode
    ADC_cfg_wr.bit.PULL_UP_EN=0; //0 : Pull-up resistor disabled on DOUT pin
    ADC_cfg_wr.bit.NOP=1; //00 : Invalid data, do not update the contents of the Config Register.  01 : Valid data, update the Config Register
    ADC_cfg_wr.bit.CNV_RDY_FL=1; //0 : Data ready, no conversion in progress  1 : Data not ready, conversion in progress
    ADC_SEL(true);
    delay_ms(30); //Ожидание для сброса предыдущих тактов SPI
    SPI_MasterTransmit(ADC_cfg_wr.byte[1]); 
    SPI_MasterTransmit(ADC_cfg_wr.byte[0]);
    ADC_cfg_rd.byte[1]=SPI_MasterTransmit(ADC_cfg_wr.byte[1]);
    ADC_cfg_rd.byte[0]=SPI_MasterTransmit(ADC_cfg_wr.byte[0]);
}

bool Read_ADS1118(unsigned char *channel) {
    if (DRDY) return false;
    if (*channel == ADC_MU) {
        if (CsuState == DISCHARGE) ADC_cfg_wr.bit.MUX = ADC_DI;
        else ADC_cfg_wr.bit.MUX = ADC_MI;
    } else {
        if ((*channel == ADC_MUp) || (!PWM_set && (PwmStatus != STOP))) ADC_cfg_wr.bit.MUX = ADC_MU;
        else ADC_cfg_wr.bit.MUX = ADC_MUp;
    }
    ADC_ADS1118[*channel].byte[1]=SPI_MasterTransmit(ADC_cfg_wr.byte[1]);
    ADC_ADS1118[*channel].byte[0]=SPI_MasterTransmit(ADC_cfg_wr.byte[0]);
    ADC_cfg_rd.byte[1]=SPI_MasterTransmit(ADC_cfg_wr.byte[1]);
    ADC_cfg_rd.byte[0]=SPI_MasterTransmit(ADC_cfg_wr.byte[0]);
    if (ADC_ADS1118[*channel].word&0x8000) ADC_ADS1118[*channel].word=0; //Если результат измерения отрицательный, то установить 0
    ADC_O[*channel]=ADC_ADS1118[*channel].word;
    if (ADC_ADS1118[*channel].word>Cfg.B[*channel]) ADC_ADS1118[*channel].word-=Cfg.B[*channel];
    else ADC_ADS1118[*channel].word=0;
    ADS1118_St[*channel]=1; //установить признак что прочитаные новые данные АЦП (данные не обработаны)
    *channel=ADC_cfg_rd.bit.MUX;
    ADC_wait=50;
    return true;
}
void clr_adc_res (void) {
    for (uint8_t i = 0; i < ADC_MI; i++)
        ADS1118_St[i] = 0;
}
#endif