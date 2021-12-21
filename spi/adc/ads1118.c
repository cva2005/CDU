#pragma message	("@(#)ads1118.c")
#include <system.h>
#include "spi/spi.h"
#include "ads1118.h"
#include "ads1118_imp.h"

static stime_t AdcTime;
static void adc_cs(cs_t cs);
static uint8_t ChCnt = CH_0;
static uint16_t AdcRes[ADC_CH];
static cfg_reg_t CfgReg = {
    BIT_RESV,
    DATA_VALID,
    PULL_UP_DIS,
    ADC_MODE,
    DR_16_SPS,
    SIGNLE_SHOT,
    PGA_2048,
    CH_0,
    UNIPOLAR,
    SINGLE_CONV
};

/*
 * —оздание структуры - описател€ SPI ADC ADS1118.
 * Configure microcontroller SPI interface to SPI
 * mode 1 (CPOL = 0, CPHA = 1).
 */
MAKE_SPI_CNTR(
    adc_cntr, /* им€ управл€ющей структуры */
    MSB_FIRST, /* пор€док следовани€ битов */
    SCK_FREQ, /* частота шины SPI, Hz */
    IDLE_LOW, /* пол€рность тактовых импульсов SPI */
    SECOND_EDGE, /* фаза тактовых импульсов SPI */
    adc_cs); /* функци€ выбора ведомого */

void adc_init (void) {
    spi_init();
    CS_OFF();
    SET_AS_OUT(CS_PORT, CS_PIN);
    spi_start_io((char *)&CfgReg, sizeof(CfgReg), 0, &adc_cntr);
}

static void adc_cs (cs_t cs) {
    ADC_SEL(cs);
}

void adc_drv (void) {
    if (IS_RDY()) { // conversion complette
        spi_get_data((char *)&AdcRes[ChCnt], sizeof(uint16_t));
        ChCnt++;
        if (ChCnt == ADC_CH) ChCnt = CH_0;
        CfgReg.mux = ChCnt;
        spi_start_io((char *)&CfgReg, sizeof(CfgReg), sizeof(uint16_t), &adc_cntr);
        AdcTime = get_fin_time(ADC_TIME);
    }
}

uint16_t get_adc_res (uint8_t ch) {
    return AdcRes[ch]; // ToDo: swap bytes
}

bool adc_error (void) {
    return get_time_left(AdcTime);
}