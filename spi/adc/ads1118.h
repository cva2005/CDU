#ifndef ADS1118_H
#define ADS1118_H
#pragma message	("@(#)ads1118.h")

#define ADC_CH      4

#define UNDERRANGE_CODE     0x0000
#define OVERRANGE_CODE      0xffff

void adc_init (void);
uint16_t get_adc_res (uint8_t ch);
void adc_drv (void);

#endif /* ADS1118_H */