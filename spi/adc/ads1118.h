#ifndef ADS1118_H
#define ADS1118_H
#pragma message	("@(#)ads1118.h")

#define ADC_CH      4

typedef struct {
    	unsigned CNV_RDY_FL     :1;
    	unsigned NOP            :2;
    	unsigned PULL_UP_EN     :1;
    	unsigned TS_MODE        :1;
    	unsigned DR             :3;
    	unsigned MODE           :1;
    	unsigned PGA            :3;
    	unsigned MUX            :2;
		unsigned MUX_POL        :1;
    	unsigned OS             :1;
} cfg_bit;

	
typedef union{
    cfg_bit bit;
    unsigned char byte[2];
    unsigned int word;
} ADS1118_type;

/*  ON-CHIP Register Selection Address*/
typedef enum {
    COMM_REG       = 0, /* Communications Register (8-bit write-only) */
    STATUS_REG     = 0, /* Status Register (8-bit read-only) */
    MODE_REG       = 1, /* Mode Register (16-bit read/write) */
    CONFIG_REG     = 2, /* Configuration Register (16-bit read/write) */
    DATA_REG       = 3, /* Data Register (16-bit read-only) */
    ID_REG         = 4, /* ID Register (8-bit read-only) */
    IO_REG         = 5, /* IO Register (8-bit read/write) */
    OFFSET_REG     = 6, /* Offset Register (16-bit read/write) */
    FULL_SCALE_REG = 7  /* Full-Scale Register (16-bit read/write)
                         * When writing to the full-scale registers, the ADC
                         * must be placed in power-down mode or idle mode!
                         */
} reg_t;

#define UNDERRANGE_CODE     0x0000
#define OVERRANGE_CODE      0xffff

#define	adc_get_data spi_get_data
#define	adc_busy spi_busy
void adc_init (void);
void adc_reset (void);
bool adc_compl (void);
void get_adc_reg (reg_t reg);
void set_adc_reg (reg_t reg, unsigned short val);
bool vrf_adc_reg (reg_t reg, char *val);
unsigned char adc_amp_gain (unsigned short inp, unsigned short ref);

void SPI_MasterInit(void);
unsigned char SPI_MasterTransmit(unsigned char cData);
void Init_ADS1118(void);
bool Read_ADS1118(unsigned char *channel);
void clr_adc_res (void);

extern ADS1118_type ADC_cfg_wr, ADC_cfg_rd;
extern ADC_Type ADC_ADS1118[ADC_CH];
extern unsigned int ADC_O[ADC_CH];
extern unsigned char ADS1118_St[ADC_CH];
extern unsigned char ADS1118_chanal, ADC_wait;
extern unsigned char ADS1118_St[ADC_CH];

#endif /* ADS1118_H */