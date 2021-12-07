/*
 * Описатель параметров конфигурации
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <sys/system.h>
#include <net/net.h>
#include "csu/csu.h"

/* адрес начала блока переменных конфигурации в RAM */
#define CFG_ADDR_START (char *)&Cfg.RsBaudRateAttr
/*
 * начало блока текущих значений счетчиков
 * (сразу за блоком параметров конфигурации)
 */
#define COUNT_EE_ADDR sizeof(CONFIG_DSC)
/* метка конца блока переменных APLY */
#define APLY_ADDR_END ((char *)&Cfg.RsDelay + 1)
/* метка начала блока переменных APLY */
#define APLY_ADDR_START (char *)&Cfg.RsBaudRate

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

/* тип структуры EEPROM */
typedef struct {
    cfg_t Cfg;
    uint8_t Crc;
} edata_t;

extern __no_init cfg_t Cfg;
void read_cfg (void);
void save_cfg (void);
uint8_t calc_crc (uint8_t *buf, uint8_t len);

#endif /* CONFIG_H */
