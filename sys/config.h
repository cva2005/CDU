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
    bf1_t bf1;
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
			uint8_t control:1;  //проконтролировать калибровку тока разряда
			uint8_t error:1;    //ошибка калибровки тока
			uint8_t up_Fin:1;   //верхний предел откалиброван
			uint8_t dw_Fin:1;   //нижний предел откалиброван
			uint8_t save:1;     //необходимо сохранить значения в EEPROM
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
    uint8_t Crc1; // ToDo: need CRC16? all zerro -> CRC8 also zerro
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
void read_num (char *num);
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
