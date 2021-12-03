/*
 * Описатель параметров конфигурации
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <sys/system.h>
#include <net/net.h>

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
    char RsBaudRateAttr;            /* атрибут параметра конфигурации */
    BAUD_RATE RsBaudRate;           /* скорость обмена по RS485 */
    char RsDataBitsAttr;            /* атрибут параметра конфигурации */
    DATA_BITS RsDataBits;           /* длина слова данных RS485 */
    char RsParityAttr;              /* атрибут параметра конфигурации */
    PARITY RsParity;                /* контроль четности RS485 */
    char RsStopBitsAttr;            /* атрибут параметра конфигурации */
    STOP_BITS RsStopBits;           /* количество стоп-битов RS485 */
    char AddrBitsAttr;              /* атрибут параметра конфигурации */
    ADDR_BITS AddrBits;             /* количество битов адреса */
    char AddresAttr;                /* атрибут параметра конфигурации */
    unsigned short Addres;          /* собственный (базовый) адрес прибора */
    char RsDelayAttr;               /* атрибут параметра конфигурации */
    unsigned char RsDelay;          /* задержка ответа от прибора по RS485, ms */
    char TcCorrAttr;                /* атрибуты параметра конфигурации */
    unsigned char TcCorr;           /* коррекция Тхс для термопар */
    char SensorAttr[INP_NUM];       /* атрибуты параметра конфигурации */
    unsigned char Sensor[INP_NUM];  /* тип датчика */
    char ChPrdAttr[INP_NUM];        /* атрибуты параметра конфигурации */
    float ChPrd[INP_NUM];           /* период опроса датчика */
    char InChAttr[INP_NUM];         /* атрибуты параметра конфигурации */
    float InCh[INP_NUM];            /* сдвиг характеристики */
    char InClAttr[INP_NUM];         /* атрибуты параметра конфигурации */
    float InCl[INP_NUM];            /* наклон характеристики */
    char InFdAttr[INP_NUM];         /* атрибуты параметра конфигурации */
    unsigned short InFd[INP_NUM];   /* постоянная времени цифрового фильтра */
    char InFgAttr[INP_NUM];         /* атрибуты параметра конфигурации */
    float InFg[INP_NUM];            /* полоса цифрового фильтра */
    char AinLoAttr[INP_NUM];        /* атрибуты параметра конфигурации */
    float AinLo[INP_NUM];           /* нижняя граница активного датчика */
    char AinHiAttr[INP_NUM];        /* атрибуты параметра конфигурации */
    float AinHi[INP_NUM];           /* верхняя граница активного датчика */
    char DpAttr[INP_NUM];           /* атрибуты параметра конфигурации */
    unsigned char Dp[INP_NUM];      /* смещение десятичной точки */
    float TcClb[2];                 /* коэфф. коррекции Тхс для термопар */
    float ClbMinR[INP_NUM];         /* коэфф. датчика положения задвижки */
    float ClbMaxR[INP_NUM];         /* коэфф. датчика положения задвижки */
} CONFIG_DSC;

extern __eeprom CONFIG_DSC eCfg;
extern __no_init CONFIG_DSC Cfg;

#endif /* CONFIG_H */
