#pragma message	("@(#)kron.c")

/*
 * Драйвер сетевого протокола KRON
 */

#include <sys/config.h>
#include "csu/csu.h"
#include "csu/mtd.h"
#include "key/key.h"
#include "lcd/lcd.h"
#include "kron.h"
#include "kron_imp.h"

static inline void TxData (void);
static void frame_parse(void);
static rx_pack_type Buff; /* буфер приема/передачи */
BUS_STATE KronBusState; /* машина сотояния приема кадра */
static unsigned char IpBuff; /* указатель данных в буфере приема */
unsigned char KronIdleCount; /* счетчик интервалов времени */
static unsigned int preset_I,  preset_Id, preset_U;

/* драйвер KRON SLAVE устройства */
void kron_drv(unsigned char ip, unsigned char len)
{
    if (KronBusState != BUS_STOP) { /* активен прием кадра */
        while (len--) {
            if (ip >= RX_BUFF_LEN) ip = 0;
            unsigned char tmp = RxBuff[ip++];
            if (tmp == CHAR_RS) { /* обнаружен стартовый байт KRON */
                IpBuff = 0; /* указатель на начало буфера */
                KronBusState = BUS_START; /* первый байт кадра принят */
            }
            if (KronBusState == BUS_START) { /* первый байт кадра уже принят */
                 if (IpBuff <= KRON_BUFF_LEN) { /* буфер не переполнен */
                    Buff.byte[IpBuff++] = tmp; /* записать байт в буфер */
                } else { /* переполнение буфера */
                    KronBusState = BUS_IDLE; /* шину в режим ожидания */
                }
            }
        }
    } else { /* KronBusState == BUS_STOP */
        if (IpBuff > KRON_RX_MIN) { /* длина кадра в норме */
            frame_parse();
        }
        KronBusState = BUS_IDLE; /* шину в режим ожидания */
    }
}

#define rx  Buff.fld.header
#define rd  Buff.fld.data
/* разбор кадра KRON */
static void frame_parse (void) {
    uint8_t cnt;
    bool multi_adr = false;
	if (rx.length <= (IpBuff - 5)) {
		if (!Cfg.bf1.PCC_ON) { //Если не установлен флаг управления с ПК
			/* если это пакет даных, то не обрабатывает его если не установлен флаг управления с ПК */
            if (rx.type == 1) return;
			/* если это запрос пользовательской конфигурации не с системного адреса */
            if ((rx.dest_adr != SYS_ADR) && (rx.type == 2)) return;
		}
        /* Если на совпадает адрес получателя, то проверить мультикастовый пакет или нет */
		if ((rx.dest_adr != Cfg.MY_ADR) && (rx.dest_adr != SYS_ADR)
            && (rx.dest_adr != BROAD_ADR)) {
			if ((rx.dest_adr & MULTI_ADR) == 0) return;
			else {
				if ((rx.dest_adr & 0x7F) != (Cfg.MY_ADR & 0x60)) return;
				else multi_adr = true;
			}
		}
		if ((rx.start == CHAR_RS) && !rx.src_adr) { //если принят верный пакет
			if (calc_crc(Buff.byte, rx.length + 4) != Buff.byte[rx.length + 4]) return;
			if (rx.length > 1) { //если в пакете есть поле типа пакета
                switch (rx.type) {
                case DATA_PKT:
					if (rx.length > 5) { //если в пакете есть информация о задаваемом токе
						if ((rd.rx_data.cmd & 0x0F) == CHARGE) { //если режим "заряд"
							if (set_I != rd.rx_data.setI) change_UI = 1;
							if (rd.rx_data.setI > Cfg.B[ADC_MI])
								preset_I = rd.rx_data.setI - Cfg.B[ADC_MI];
							else preset_I = 0;
							if (preset_I > max_set_I) preset_I = max_set_I;
						}
						if ((rd.rx_data.cmd & 0x0F) == DISCHARGE) { //если режим "разряд"
							if (set_Id != rd.rx_data.setI) change_UI = 1;
							if (rd.rx_data.setI > Cfg.B[ADC_DI])
								preset_Id = rd.rx_data.setI - Cfg.B[ADC_DI];
							else preset_Id = 0;
							if (preset_Id > max_set_Id) preset_Id = max_set_Id;
						}
					}
					if (rx.length > 7) { //в пакете есть данные о задаваемом напряжении?
						if (set_U != rd.rx_data.setU) change_UI = 1;
						if (rd.rx_data.setU > Cfg.B[ADC_MU])
							preset_U = rd.rx_data.setU - Cfg.B[ADC_MU];
						else preset_U = 0;
						if (preset_U > max_set_U) preset_U = max_set_U;
					}
					if (rx.length > 3) { //если в пакете есть контрольные биты
						if (Cfg.bf1.FAN_CONTROL) { //Если разрешён контроль выходных реле и вентиляторов
							if (rd.rx_data.control.bit.FAN1_ON) FAN(1);
							else FAN(0);
						}
					}
					if (rx.length > 2) { //если в пакете данных есть команда
						if (rd.rx_data.cmd&0x0F) { //Если это команда запуска разряда или заряда
							if (!(rd.rx_data.cmd & 0xF0)) { //Если пришла команда со сброшенным флагом отсрочки выполнения (0x0x: 0x01 или 0x02)
                              if (rd.rx_data.cmd == 0x03) {
									key_power();
								} else {
									set_I = preset_I; //установить зарядный ток
									set_Id = preset_Id; //установить разрядный ток
									set_U = preset_U; //установить напряжение
									if (CSU_Enable != rd.rx_data.cmd) {
										self_ctrl = 0;
										Start_CSU(rd.rx_data.cmd); //если изменилась комнада, то запустить блок с новой командой	
									}
								}
							}
						} else {
							if (((CSU_Enable|RELAY_EN) != rd.rx_data.cmd) || (Error != 0))
                                Stop_CSU(rd.rx_data.cmd);
						}	
					}
                    break;
                case USER_CFG:
					if (rx.length >= 12) { //если поле данных конфигурирования не пустое
						if (rd.rx_usr.cmd.bit.ADR_SET) {
							if ((rd.rx_usr.Adress!=0)&&(rd.rx_usr.Adress!=0xFF)) 
								Cfg.MY_ADR = rd.rx_usr.Adress;	
						}
						Cfg.K_I = rd.rx_usr.K_I;
						Cfg.K_U = rd.rx_usr.K_U;
						Cfg.K_Ip = rd.rx_usr.K_Ip;
						Cfg.K_Id = rd.rx_usr.K_Id;
						if (rx.length >= 20) {				
							Cfg.B[ADC_MI] = rd.rx_usr.B_I;		
							Cfg.B[ADC_MU] = rd.rx_usr.B_U;
							Cfg.B[ADC_MUp] = rd.rx_usr.B_Ip;
							Cfg.B[ADC_DI] = rd.rx_usr.B_Id;
						}
						calc_cfg();		
						if (rd.rx_usr.cmd.bit.EEPROM) save_cfg();
					}
                    break;
                case SYS_CFG:
					if (rx.length >= 0x0C) { //если поле данных конфигурирования не пустое
						rd.rx_sys.mode.bit.RELAY_MODE = 1;
						rd.rx_sys.cmd.bit.TE_DATA = 0;	
						((uint8_t *)&Cfg.bf1)[0] = rd.rx_sys.cmd.byte;
						((uint8_t *)&Cfg.bf1)[1] = rd.rx_sys.mode.byte;
						/* Защита от одновременного включения LCD и LED (LCD Приоритет) */
                        if (Cfg.bf1.LCD_ON) Cfg.bf1.LED_ON = 0;
						Cfg.maxU = rd.rx_sys.maxU;
						Cfg.maxI = rd.rx_sys.maxI;
						Cfg.maxId = rd.rx_sys.maxId;
						Cfg.P_maxW = rd.rx_sys.maxPd;
						if (rx.length >= 0x0D) //Если есть данные о количестве РМ
							Cfg.DM_SLAVE = rd.rx_sys.dm_cnt;
						if (rx.length >= 17) //если есть поле дополнительной конфигурации
							*((uint16_t *)&Cfg.bf2) = rd.rx_sys.cfg;	
						if (rx.length >= 22) { //если есть информация об автозапуске
							Cfg.cnt_set = rd.rx_sys.autostart_try;
							Cfg.u_set = rd.rx_sys.autostart_u;
							Cfg.time_set = rd.rx_sys.restart_timout/16;
						}
						calc_cfg();
						if (rd.rx_sys.cmd.bit.EEPROM) save_cfg();
					}
                    break;
                case VER_PKT:
					if (rx.length == 0x0C) { //если поле данных конфигурирования не пустое
						if (rd.rx_ver.cmd.bit.EEPROM!=0)
							save_num(&rd.rx_ver.number[0]);
						if (Cfg.bf1.LCD_ON) LCD_wr_connect(1);
					}						
                    break;
                case ALG_PKT:
					if (CSU_Enable != 0) Stop_CSU(0);
                    switch (rd.rx_alg.cmd) {
                    case FIND_CMD:
						mCnt = sCnt = cCnt = 0;//номера метода, этапа и цикла
						for (cnt = 0; cnt < MTD_N; cnt++) StageNum[cnt] = 0;
						WrNum = find_free();
                        break;
                    case DEL_CMD:
						delete_all_method();
						WrNum = 0;
                        break;
                    case SAVE_CMD:
						if (WrNum < MS_N) {
							save_alg(WrNum, &rd.rx_alg.data[0]);
							WrNum++;
						}
                        break;
                    case RST_CMD:
                        WrNum = 0;
                    }
                    break;
                case EEPR_PKT:
					WrNum = 0;
                    break;
                default:
                    /* Если нет поля с типом пакета (пакет нулевой длины),
                    то считать что это запрос данных */
                    rx.type = DATA_PKT;
                }
            }
            if ((rx.dest_adr != BROAD_ADR) && !multi_adr) TxData();
        }
    }
}

static inline void TxData (void) {
}
