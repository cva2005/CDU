#pragma message	("@(#)config.c")
#include <sys/config.h>

/* EEPROM параметры конфигураци */
static __eeprom edata_t eData;

/* RAM копия параметров конфигураци */
__no_init cfg_t Cfg;
__no_init num_t Num;
__no_init clb_t Clb;

/* Default Config Data */
static const cfg_t DefaultCfg = {
    K_U_DEF, // uint16_t K_U;
    K_I_DEF, // int16_t K_I;
    K_ID_DEF, // uint16_t K_Id;
    K_UP_DEF, // uint16_t K_Ip;
    MAX_U_DEF, // uint16_t maxU;
    MAX_I_DEF, // uint16_t maxI;
    MAX_ID_DEF, // uint16_t maxId;
    MAX_PD_DEF, // uint16_t P_maxW;
    OFF, // unsigned EEPROM :1;
	OFF, //unsigned ADR_SET :1;
	IN_DATA_B, // unsigned IN_DATA :1;
	OUT_DATA_B, // unsigned OUT_DATA :1;
	OFF, // unsigned TE_DATA :1;
	OFF, // unsigned FAN_CONTROL :1;
	DIAG_WIDE_B, // unsigned DIAG_WIDE :1;
	I0_SENSE_B, // unsigned I0_SENSE :1;
	LCD_ON_B, // unsigned LCD_ON :1;
	LED_ON_B, // unsigned LED_ON :1;
	PCC_ON_B, // unsigned PCC_ON :1;
	DEBUG_ON_B, // unsigned DEBUG_ON :1;
	GROUP_B, // unsigned GroupM :1;
	EXT_ID_B, // unsigned EXT_Id :1;
	OFF, // unsigned EXTt_pol :1;
	RELAY_MODE_B, // unsigned RELAY_MODE :1;
    DM_EXT, // uint8_t dmSlave;
    ADDR_DEF, // uint8_t addr;
    B_U_DEF,
    B_I_DEF,
    B_ID_DEF,
    B_UP_DEF,
    A_START_B, // autostart :1;
    OFF, // cdu_dsch_dsb :1;
    OFF, // pulse mode :1;
    AST_TIME, // uint16_t time_set;
    AST_U, // uint16_t u_set;
    AST_CNT // uint8_t cnt_set;
};

void read_cfg (void) {
    Cfg = eData.Cfg;
    uint8_t crc = eData.Crc1;
    if (crc != calc_crc((uint8_t *)&Cfg, sizeof(Cfg))) {
        Cfg = DefaultCfg;
        save_cfg();
    }
    calc_cfg();
    Num = eData.Num;
    crc = eData.Crc2;
    if (crc != calc_crc((uint8_t *)&Num, sizeof(Num)))
        memset((void *)&Num, 0xff, sizeof(Num));
}

bool read_clb (void) {
    Clb = eData.Clb;
    uint8_t crc = eData.Crc3;
    if (crc == calc_crc((uint8_t *)&Clb, sizeof(Clb))) return true;
    else return false;
}

void save_clb (void) {
    eData.Clb = Clb;
    eData.Crc3 = calc_crc((uint8_t *)&Clb, sizeof(Clb));
}

void save_cfg (void) {
    eData.Cfg = Cfg;
    eData.Crc1 = calc_crc((uint8_t *)&Cfg, sizeof(Cfg));
}

void read_num (char *dest) {
    memcpy(dest, (void *)&Num, sizeof(Num));
}

void save_num (uint8_t *src) {
    memcpy((void *)&Num, src, sizeof(Num));
    eData.Num = Num;
    eData.Crc2 = calc_crc((uint8_t *)&Num, sizeof(Num));
}

bool eeread_mtd (uint8_t num, mtd_t *pm) {
    *pm = eData.MtdStg[num].data;
    uint8_t crc = eData.MtdStg[num].crc;
    if (crc == calc_crc((uint8_t *)pm, sizeof(mtd_t))) {
        if (pm->fld.data_type == MTD_ID) return true;
    }
    return false;
}

bool eeread_stg (uint8_t num, stg_t *ps) {
    *((mtd_t *)ps) = eData.MtdStg[num].data;
    uint8_t crc = eData.MtdStg[num].crc;
    if (crc == calc_crc((uint8_t *)ps, sizeof(stg_t))) {
        if (ps->fld.data_type == STG_ID) return true;
    }
    return false;
}

void save_alg (uint8_t num, void *p) {
    eData.MtdStg[num].data = *((mtd_t *)p);
    eData.MtdStg[num].crc = calc_crc((uint8_t *)p, sizeof(mtd_t));
}

void eeclr_alg (void) {
    uint8_t n = 0, i = MS_N;
    while (--i) {
        if (eData.MtdStg[i].crc != CLR_ID
            && eData.MtdStg[i].data.fld.data_type != CLR_ID) {
            if (n++ % 2) eData.MtdStg[i].data.fld.data_type = CLR_ID;
            else eData.MtdStg[i].crc = CLR_ID;
        }
    }
}

/* Вычисление контрольной суммы CRC8 */
uint8_t calc_crc(uint8_t *buf, uint8_t len) {
    uint8_t crc = 0, i, b, n, d;
    for (i = 0; i < len; i++) {
        b = 8; d = buf[i];
        do {
            n = (d ^ crc) & 0x01;
            crc >>= 1; d >>= 1;
            if (n) crc ^= 0x8C;
        } while(--b);
    }
    return crc;
}