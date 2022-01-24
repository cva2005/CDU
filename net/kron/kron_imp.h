#ifndef KRON_IMP_H
#define KRON_IMP_H
#pragma message	("@(#)kron_imp.h")

/*
 * Драйвер сетевого протокола KRON
 */

#ifdef	__cplusplus
extern "C" {
#endif

#define KRON_RX_MIN         4 /* минимальная длина кадра */
#define KRON_RX_MAX         250 /* максимальная длина кадра */
#define CHAR_TS             0x5A /* символ начала кадра */
#define CHAR_RS             0xA5 /* символ начала кадра */
#define KRON_BUFF_LEN       KRON_RX_MAX
#define RX_LEN              KRON_BUFF_LEN
#define TX_LEN              50
#define BUFF_SIZE           RX_LEN > TX_LEN ? RX_LEN : TX_LEN
#define SYS_ADR             0xFF
#define BROAD_ADR           0xFE
#define MULTI_ADR           0x80

/* packet type */
#define DATA_PKT            1
#define USER_CFG            2
#define SYS_CFG             3
#define VER_PKT             4
#define ALG_PKT             5
#define EEPR_PKT            6

/* command type */
#define FIND_CMD            1
#define DEL_CMD             0x20 // ToDo: or 2?
#define SAVE_CMD            3
#define RST_CMD             4

typedef struct {
    uint8_t fan1    :1;
    uint8_t fan2    :1;
    uint8_t rsrv    :6;
} fcntl_t;

typedef	struct {
    uint8_t start;
    uint8_t dest_adr;
    uint8_t src_adr;
    uint8_t length;
    uint8_t number;
    uint8_t type;
} header_type;

typedef	struct {
    uint8_t cmd;
    fcntl_t fcntl;
    uint16_t setI;
    uint16_t setU;
} rdata_t;
	
typedef	struct {
    uint8_t operation;
    uint8_t error;
    uint16_t I;
    uint16_t U;
    uint16_t Ip;
    uint16_t t1;
    uint16_t t2;
    uint8_t In_st;
    uint8_t Out_st;
} tdata_t;

typedef	struct {
    cmd_t cmd;
    uint8_t addr;
    uint16_t K_I;
    uint16_t K_U;
    uint16_t K_Ip;
    uint16_t K_Id;
    uint16_t B_I;
    uint16_t B_U;
    uint16_t B_Ip;
    uint16_t B_Id;
    uint8_t D_I;
    uint8_t D_U;
    uint8_t D_Ip;
    uint8_t D_Id;
} usr_t;
	
typedef	struct {
    cmd_t cmd;
    mode_t mode;
    uint16_t maxU;
    uint16_t maxI;
    uint16_t maxId;
    uint16_t maxPd;
    uint8_t dm_cnt;
    uint8_t slave_cnt_u;
    uint8_t slave_cnt_i;
    uint16_t cfg;
    uint8_t autostart_try;
    uint16_t restart_timout;
    uint16_t autostart_u;
} sys_t;
	
typedef	struct {
    cmd_t cmd;
    uint8_t rsrv;
    uint8_t num[8];
} rver_t;

typedef	struct {
    uint8_t hard_ver;
    uint8_t hard_mode;
    uint8_t soft_ver;
    uint8_t soft_mode;
    char number[8];
} tver_t;

typedef	struct {
    uint8_t cmd;	
    uint8_t size;
    uint8_t data[32];
} algr_t; 
	
typedef	struct {
    uint8_t res;
    uint16_t pmem;
} algt_t;

typedef	struct {
    uint16_t adr;
    uint8_t d[32];
} ee_t;

typedef union {
    struct {
        header_type header;
        union {
            sys_t sys;
            usr_t usr;
            rdata_t rdata;
            tdata_t tdata;
            rver_t rver;
            tver_t tver;
            algr_t algr;
            algt_t algt;
            ee_t eepr;
		} data;
	} fld;
	uint8_t byte[BUFF_SIZE];
} rs_pkt_t;

#ifdef __cplusplus
}
#endif

#endif /* KRON_IMP_H */
