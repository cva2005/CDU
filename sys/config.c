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
    K_U_const, // uint16_t K_U;
    K_I_const, // int16_t K_I;
    K_Id_const, // uint16_t K_Id;
    K_Up_const, // uint16_t K_Ip;
    maxU_const, // uint16_t maxU;
    maxI_const, // uint16_t maxI;
    maxId_const, // uint16_t maxId;
    maxPd_const, // uint16_t P_maxW;
    0, // unsigned EEPROM :1;
	0, //unsigned ADR_SET :1;
	IN_DATA_bit, // unsigned IN_DATA :1;
	OUT_DATA_bit, // unsigned OUT_DATA :1;
	0, // unsigned TE_DATA :1;
	0, // unsigned FAN_CONTROL :1;
	DIAG_WIDE_bit, // unsigned DIAG_WIDE :1;
	I0_SENSE_bit, // unsigned I0_SENSE :1;
	LCD_ON_bit, // unsigned LCD_ON :1;
	LED_ON_bit, // unsigned LED_ON :1;
	PCC_ON_bit, // unsigned PCC_ON :1;
	DEBUG_ON_bit, // unsigned DEBUG_ON :1;
	GroupM_bit, // unsigned GroupM :1;
	EXT_Id_bit, // unsigned EXT_Id :1;
	0, // unsigned EXTt_pol :1;
	RELAY_MODE_bit, // unsigned RELAY_MODE :1;
    DM_ext, // uint8_t dmSlave;
    MY_ADR_const, // uint8_t MY_ADR;
    B_U_const,
    B_I_const,
    B_Id_const,
    B_Up_const,
    AUTOSTART, // unsigned char autostart :1;
    0, // unsigned char cdu_dsch_dsb :1;
    AUTOSTART_TIME, // uint8_t time_set;
    AUTOSTART_U, // uint16_t u_set;
    AUTOSTART_CNT, // uint8_t cnt_set;
    0 // uint8_t Reserved;
};

void read_cfg (void) {
    Cfg = eData.Cfg;
    uint8_t crc = eData.Crc1;
    if (crc != calc_crc((uint8_t *)&Cfg, sizeof(Cfg)))
        Cfg = DefaultCfg;
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

bool read_mtd (uint8_t num, mtd_t *pm) {
    *pm = eData.MtdStg[num].data;
    uint8_t crc = eData.MtdStg[num].crc;
    if (crc == calc_crc((uint8_t *)pm, sizeof(mtd_t))) {
        if (pm->fld.data_type == MTD_ID) return true;
    }
    return false;
}

void save_alg (uint8_t num, void *p) {
    eData.MtdStg[num].data = *((mtd_t *)p);
    eData.MtdStg[num].crc = calc_crc((uint8_t *)p, sizeof(mtd_t));
}

bool read_stg (uint8_t num, stg_t *ps) {
    *((mtd_t *)ps) = eData.MtdStg[num].data;
    uint8_t crc = eData.MtdStg[num].crc;
    if (crc == calc_crc((uint8_t *)ps, sizeof(stg_t))) {
        if (ps->fld.data_type == STG_ID) return true;
    }
    return false;
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