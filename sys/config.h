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
    CSU_cfg_bit bf1;
    uint8_t DM_SLAVE;
    uint8_t MY_ADR;
    uint16_t B[4];
    CSU2_cfg_bit bf2; 
    uint8_t time_set;
    uint16_t u_set;
    uint8_t cnt_set;
    uint8_t Reserved;
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
			uint8_t control:1;	//проконтролировать калибровку тока разряда
			uint8_t error:1;	    //ошибка калибровки тока
			uint8_t up_finish:1;	//верхний предел откалиброван
			uint8_t dw_finish:1;	//нижний предел откалиброван
			uint8_t save:1;			//необходимо сохранить значения в EEPROM
		} bit;
		uint8_t byte;
	} id;
	uint8_t crc8;
} clb_t;

#define EEPROM_SIZE     1024
#define MS_SIZE sizeof(stg_t) > sizeof(mtd_t) ? sizeof(stg_t) : sizeof(mtd_t)

/* metod/stage type */
typedef struct {
    mtd_t data;
    uint8_t crc;
} ms_t;

#define MS_N (EEPROM_SIZE - sizeof(Cfg) - sizeof(Num)\
     - sizeof(uint8_t) * 3) / MS_SIZE
/* тип структуры EEPROM */
typedef struct {
    cfg_t Cfg;
    uint8_t Crc1;
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
void read_num (uint8_t *num);
void save_num (uint8_t *src);
bool read_clb (void);
bool read_mtd (uint8_t num, mtd_t *pm);
bool read_stg (uint8_t num, stg_t *ps);
void save_alg (uint8_t num, void *p);
void save_cfg (void);
void save_clb (void);
uint8_t calc_crc (uint8_t *buf, uint8_t len);

#define MTD_ID     0xAA
#define STG_ID     0x55

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
