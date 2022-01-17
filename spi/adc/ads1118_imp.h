#ifndef ADS1118_IMP_H
#define ADS1118_IMP_H
#pragma message	("@(#)ads1118_imp.h")
//( Config Register )
//Operational status/single-shot conversion start
#define     CONFIG_BIT_OS           (1 << 15)
//MUX[2:0]: Input multiplexer configuration
#define     CONFIG_BIT_MUX          (7 << 12)
//PGA[2:0]: Programmable gain amplifier configuration
#define     CONFIG_BIT_PGA          (7 << 9)
//MODE: Device operating mode
#define     CONFIG_BIT_MODE         (1 << 8)
//DR[2:0]: Data rate
#define     CONFIG_BIT_DR           (7 << 5)
//TS_MODE: Temperature sensor mode
#define     CONFIG_BIT_TS_MODE      (1 << 4)
//PULL_UP_EN: Pull-up enable
#define     CONFIG_BIT_PULLUP_EN    (1 << 3)
//NOP: No operation
#define     CONFIG_BIT_NOP          (3 << 1)
//CONFIG_BIT_RESV: default 
#define     CONFIG_BIT_RESV         (1 << 0)

#define ADS1118_CONST_6_144V_LSB_mV  (0.1875)
#define ADS1118_CONST_4_096V_LSB_mV  (0.125)
#define ADS1118_CONST_2_048V_LSB_mV  (0.0625)
#define ADS1118_CONST_1_024V_LSB_mV  (0.03125)
#define ADS1118_CONST_0_512V_LSB_mV  (0.015625)
#define ADS1118_CONST_0_256V_LSB_mV  (0.0078125)

#define SAMPLECOUNTER		9
#define LEN                 2
#define CH_0                0
#define CH_1                1
#define BIT_RESV            0

typedef union {
    struct {
        volatile unsigned RESV      :1; //low
        volatile unsigned NOP       :2;
        volatile unsigned PULLUP    :1;
        volatile unsigned TS_MODE   :1;
        volatile unsigned DR        :3;
        volatile unsigned MODE      :1;
        volatile unsigned PGA       :3;
        volatile unsigned MUX       :3;
        volatile unsigned OS        :1; //high
    } bf_t;
    volatile uint16_t word;
    volatile uint8_t byte[2];
} adc_t;

typedef struct {
    unsigned mode      :1;
    unsigned pga       :3;
    unsigned mux       :2;
    unsigned pol       :1;
    unsigned ss        :1; //high
    unsigned resv      :1; //low
    unsigned nop       :2;
    unsigned pull      :1;
    unsigned ts_m      :1;
    unsigned dr        :3;
} cfg_reg_t;

typedef enum {
    CONVERING   = 0x1, //for read
    SINGLE_CONV = 0x1  //for write
} ss_t;

typedef enum {
    AINPN_0_1 	= 	0x0,
    AINPN_0_3 	=   0x1,
    AINPN_1_3 	=   0x2,
    AINPN_2_3 	=   0x3,
    AINPN_0_GND	=  	0x4,
    AINPN_1_GND	=  	0x5,
    AINPN_2_GND	=  	0x6,
    AINPN_3_GND	=  	0x7
} mux_t;

typedef enum {
    PGA_6144 	= 0x0,
    PGA_4096 	= 0x1,
    PGA_2048 	= 0x2,
    PGA_1024 	= 0x3,
    PGA_512 	= 0x4,
    PGA_256 	= 0x5
} pga_t;

typedef enum {
    CONTIOUS    =  0x0,
    SIGNLE_SHOT =  0x1
} mode_t;

typedef enum {
    DR_8_SPS   =   0x0,
    DR_16_SPS  =   0x1,
    DR_32_SPS  =   0x2,
    DR_64_SPS  =   0x3,
    DR_128_SPS =   0x4,
    DR_250_SPS =   0x5,
    DR_475_SPS =   0x6,
    DR_860_SPS =   0x7
} rate_t;

typedef enum {
    ADC_MODE         = 0x0,
    TEMPERATURE_MODE = 0x1
} ts_t;

typedef enum {
    BIPOLAR         = 0x0,
    UNIPOLAR        = 0x1
} pol_t;

typedef enum {
    PULL_UP_DIS = 0x0,
    PULL_UP_EN  = 0x1
} pull_t;

typedef enum {
    DATA_VALID      = 0x1,
    DATA_INVALID    = 0x2
} nop_t;

typedef enum {
    DATA_READY 	= 0x0,
    DATA_NREADY = 0x1
} rdy_t;

/* ADC Max Clock Frequency = 4000 kHz */
#define SCK_FREQ        4000001UL
#define CS_PORT         A
#define CS_PIN          2
#define CS_ON()         CLR_PIN(CS_PORT, CS_PIN)
#define CS_OFF()        SET_PIN(CS_PORT, CS_PIN)
#define ADC_SEL(x)      !x ? (CS_OFF()) : (CS_ON())
/* линия RYD (она же MISO) */
#define RDY_PORT        B
#define RDY_PIN         6
#define IS_RDY()        IS_PIN_CLR(RDY_PORT, RDY_PIN)
//#define DRDY            PINB & (1 << 6)
#define ADC_TIME        MS(200)

#endif /* ADS1118_IMP_H */