#pragma message	("@(#)ads1118.c")
#include "sys/system.h"
#include "net/net.h"
#include "csu/csu.h"
#include "pwm/pwm.h"
#include "ads1118.h"

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
    ADC_Sel(0);
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
