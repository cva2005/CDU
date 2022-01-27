#pragma message	("@(#)ads1118.c")
#include <system.h>
#include "spi/spi.h"
#include "ads1118.h"
#include "ads1118_imp.h"

static bool AdcBusy;
static bool AdcReset = false;
static stime_t AdcTime, ResTime;
static void adc_cs(cs_t cs);
static mux_t ChCnt = CH_3;
static uint16_t AdcRes[ADC_CH];
static cfg_reg_t CfgReg = {
    BIT_RESV,
    DATA_VALID,
    PULL_UP_DIS,
    ADC_MODE,
    DR_250_SPS,
    SIGNLE_SHOT,
    PGA_2048,
    CH_0,
    UNIPOLAR,
    SINGLE_CONV
};
#define ADC_DEBUG   0
#if ADC_DEBUG
static cfg_reg_t CfgRd;
#endif

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
    //CS_ON(); // 32-Bit Data Transmission Cycle (CS -> ON)
    //SET_AS_OUT(CS_PORT, CS_PIN);
    spi_start_io((char *)&CfgReg, sizeof(uint16_t), sizeof(uint32_t), &adc_cntr);
    AdcTime = get_fin_time(ADC_TIME);
}

static void adc_cs (cs_t cs) {
    if (cs) AdcBusy = true;
    else AdcBusy = false;
    //ADC_SEL(cs);
}

/*
 * If SCLK is held low for 28 ms,
 * the serial interface resets and the next SCLK pulse starts a new
 * communication cycle. This time-out feature can be used to
 * recover communication when a serial interface transmission
 * is interrupted. When the serial interface is idle, hold SCLK low.
 */

void adc_drv (void) {
    if (IS_RDY() && !AdcBusy) { // conversion complette
        int16_t res; uint16_t b = Cfg.B[ChCnt];
        spi_get_data((char *)&res, 0, sizeof(uint16_t));
        if (res < 0) res = 0;
        ADC_O[ChCnt] = res;
        if (res > b) res -= b;
        else res = 0;
        AdcRes[ChCnt] = res;
#if ADC_DEBUG
        spi_get_data((char *)&CfgRd, sizeof(uint16_t), sizeof(uint16_t));
#endif
        ChCnt++;
        if (ChCnt == CH_3) {
            CfgReg.mux = CH_0;
        } else {
            if (ChCnt == ADC_CH) ChCnt = CH_0;
            CfgReg.mux = (mux_t)(ChCnt + 1);
        }
        spi_start_io((char *)&CfgReg, sizeof(uint16_t), sizeof(uint32_t), &adc_cntr);
        goto adc_start;
    } else if (adc_error()) {
        if (AdcReset) {
            if (!get_time_left(ResTime)) {
                AdcReset = false;
                adc_init();
            adc_start:
                AdcTime = get_fin_time(ADC_TIME);
           }
        } else {
            AdcReset = true;
            spi_reset();
            ResTime =  get_fin_time(RES_TIME);
        }
    }
}

uint16_t get_adc_res (uint8_t ch) {
    return AdcRes[ch];
}

bool adc_error (void) {
    return (!get_time_left(AdcTime));
}