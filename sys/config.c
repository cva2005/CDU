/*
 * Описатель параметров конфигурации
 */

#include <sys/config.h>

/* EEPROM параметры конфигураци */
static __eeprom edata_t eData;

/* RAM копия параметров конфигураци */
__no_init cfg_t Cfg;

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
    DM_ext, // uint8_t DM_SLAVE;
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
    uint8_t crc = eData.Crc;
    if (crc != calc_crc((uint8_t *)&Cfg, sizeof(Cfg)))
        Cfg = DefaultCfg;
}

void save_cfg (void) {
    eData.Cfg = Cfg;
    eData.Crc = calc_crc((uint8_t *)&Cfg, sizeof(Cfg));
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