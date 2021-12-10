#ifndef ADS1118_H_
#define ADS1118_H_
#pragma message	("@(#)ads1118.h")


//#define DDR_SPI PORTB
//#define DD_MOSI PORTB5
//#define DD_SCK PORTB7

#define ADC_Sel(x) ((x)?(PORTA|=(1<<2)):(PORTA&=~(1<<2)))
#define DRDY (PINB&(1<<6))

typedef
  	struct{
    	unsigned char CNV_RDY_FL :1;
    	unsigned char NOP :2;
    	unsigned char PULL_UP_EN :1;
    	unsigned char TS_MODE :1;
    	unsigned char DR :3;
    	unsigned char MODE :1;
    	unsigned char PGA :3;
    	unsigned char MUX :2;
		unsigned char MUX_POL :1;
    	unsigned char OS :1;
   }cfg_bit;

	
typedef union{
	cfg_bit bit;
	unsigned char byte[2];
	unsigned int word;
	} ADS1118_type;

void SPI_MasterInit(void);
unsigned char SPI_MasterTransmit(unsigned char cData);
void Init_ADS1118(void);
unsigned char Read_ADS1118(unsigned char *channel);
extern ADS1118_type ADC_cfg_wr, ADC_cfg_rd;
extern ADC_Type ADC_ADS1118[4];
extern unsigned int ADC_O[4];
extern unsigned char ADS1118_St[4];
extern unsigned char ADS1118_chanal, ADC_finish, ADC_wait;
extern unsigned char ADS1118_St[4];

#endif /* ADS1118_H_ */