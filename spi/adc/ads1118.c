#pragma message	("@(#)ads1118.c")
#include <system.h>
#include "spi/spi.h"
#include "ads1118.h"
#include "ads1118_imp.h"

static bool AdcBusy;
static stime_t AdcTime;
static void adc_cs(cs_t cs);
static uint8_t ChCnt = CH_0;
static uint16_t AdcRes[ADC_CH];
static cfg_reg_t CfgReg = {
    SIGNLE_SHOT,
    PGA_2048,
    CH_0,
    UNIPOLAR,
    SINGLE_CONV,
    BIT_RESV,
    DATA_VALID,
    PULL_UP_DIS,
    ADC_MODE,
    DR_16_SPS
};

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

void adc_init (void) {
    spi_init();
    CS_ON(); // 32-Bit Data Transmission Cycle (CS -> ON)
    SET_AS_OUT(CS_PORT, CS_PIN);
    spi_start_io((char *)&CfgReg, sizeof(uint16_t), sizeof(uint32_t), &adc_cntr);
}

static void adc_cs (cs_t cs) {
    if (cs) AdcBusy = true;
    else AdcBusy = false;
    //ADC_SEL(cs);
}

void adc_drv (void) {
    if (IS_RDY() && !AdcBusy) { // conversion complette
        spi_get_data((char *)&AdcRes[ChCnt], sizeof(uint16_t));
        ChCnt++;
        if (ChCnt == ADC_CH) ChCnt = CH_0;
        CfgReg.mux = ChCnt;
        spi_start_io((char *)&CfgReg, sizeof(uint16_t), sizeof(uint32_t), &adc_cntr);
        AdcTime = get_fin_time(ADC_TIME);
    }
}

uint16_t get_adc_res (uint8_t ch) {
    return AdcRes[ch]; // ToDo: swap bytes
}

bool adc_error (void) {
    return get_time_left(AdcTime);
}