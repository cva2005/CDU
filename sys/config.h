#ifndef CONFIG_H
#define CONFIG_H
#pragma message	("@(#)config.h")

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/system.h>
#include <net/net.h>
#include "csu/csu.h"
#include "csu/mtd.h"

/* Default Configuration */
#define A_START_B       OFF
#define AST_CNT         100
#define AST_TIME        1600
#define AST_U           0
#define ADDR_DEF        1
#define IN_DATA_B       OFF
#define OUT_DATA_B      OFF
#define DIAG_WIDE_B     ON
#define I0_SENSE_B      ON
#define LCD_ON_B        ON
#define LED_ON_B        OFF
#define PCC_ON_B        ON
#define DEBUG_ON_B      OFF
#define GROUP_B         OFF
#define EXT_ID_B        OFF
#define RELAY_MODE_B    ON
#define DM_EXT          0
#define MTD_DEF         1 // - к-во методов по ум-ю (0 - 1 метод)
#define MAX_U_DEF       4000 //ограничение напряжения		xx00
#define MAX_I_DEF       4500 //ограничение тока				xx00
#define MAX_ID_DEF      1800 //ограничение тока разряда		xx00
#define MAX_PD_DEF      2000 //ограничение мощности разряда	xxx0
#define MAX_ID_EXT0     2000 //максимальны16226й ток разряда если нет разрядных модулей
#define MAX_ID_EXT1     5000 //максимальный ток разряда если есть разрядные модули
#define K_U_DEF         12730 //12615//12650  //R1=26700+1500 R2=1500 Uadc=2,048 Umax=40,55V (K=405500000/(32768-1147)=12375)
#define	K_UP_DEF        12620 //12465//12532  //
#define K_I_DEF         15516 //15692//15589   //R1=10000 R2=383 Uadc=2,048 Ushunt=75.55mV Imax=75.55*50/75=50.37A (K=503666667/32768=15371)
#define K_ID_DEF        16226 //16256//15816  //R1=10000 R2=383 Uadc=2,048 Ushunt=78.45mV Idmax=78.45*50/75=52.3A (K=523000000/32768=15961)
#define B_U_DEF         1147 //R1=26700; R2=383; Ushift=0.070709; B_U=Ushift*32767/2.048=1131.31
#define B_UP_DEF        60 //Смещение нулевого значения АЦП при измерении U до выходного реле
#define B_I_DEF         0 //Смещение нулевого значения АЦП по току заряда
#define B_ID_DEF        13 //Смещение нулевого значения АЦП по току разряда

/* тип структуры параметров конфигураци */
typedef struct {
    uint16_t K_U;
    uint16_t K_I;
    uint16_t K_Id;
    uint16_t K_Ip;
    uint16_t maxU;
    uint16_t maxI;
    uint16_t maxId;
    uint16_t P_maxW;
    cmd_t cmd;
    mode_t mode;
    uint8_t dmSlave;
    uint8_t addr;
    uint16_t B[4];
    bf2_t bf2; 
    uint16_t time_set;
    uint16_t u_set;
    uint8_t cnt_set;
} cfg_t;

#define N_CH        8 // chars in work number
/* Work number */
typedef struct {
    uint8_t w[N_CH];
} num_t;

typedef struct {
    int16_t pwm1;
	int16_t setI1;
	int16_t pwm2;
	int16_t setI2;
	union {
		struct {
			uint8_t control :1; //проконтролировать калибровку тока разряда
			uint8_t error   :1; //ошибка калибровки тока
			uint8_t up_Fin  :1; //верхний предел откалиброван
			uint8_t dw_Fin  :1; //нижний предел откалиброван
			uint8_t save    :1; //необходимо сохранить значения в EEPROM
		} bit;
		uint8_t byte;
	} id;
} clb_t;

#define EEPROM_SIZE     1024
#define MS_SIZE         sizeof(ms_t)

/* metod/Stg type */
typedef struct {
    mtd_t data;
    uint8_t crc;
} ms_t;

#define CFG_SIZE (sizeof(cfg_t) + sizeof(num_t) +\
    sizeof(clb_t) + sizeof(uint8_t) * 3)
#define MS_N (EEPROM_SIZE - CFG_SIZE) / MS_SIZE
/* тип структуры EEPROM */
typedef struct {
    cfg_t Cfg;
    uint8_t Crc1; // ToDo: need CRC16, if data[] == {0} -> CRC8 = 0
    num_t Num;
    uint8_t Crc2;
    clb_t Clb;
    uint8_t Crc3;
    ms_t MtdStg[MS_N];
} edata_t;

extern __no_init cfg_t Cfg;
extern __no_init num_t Num;
extern __no_init clb_t Clb;
void read_cfg (void);
void read_num (char *dest);
void save_num (uint8_t *src);
bool read_clb (void);
bool eeread_mtd (uint8_t num, mtd_t *pm);
bool eeread_stg (uint8_t num, stg_t *ps);
void save_alg (uint8_t num, void *p);
void save_cfg (void);
void save_clb (void);
void eeclr_alg (void);
uint8_t calc_crc (uint8_t *buf, uint8_t len);

#define MTD_ID     0xAA
#define STG_ID     0x55
#define CLR_ID     0xFF

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
