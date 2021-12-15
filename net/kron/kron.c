#pragma message	("@(#)kron.c")

/*
 * Драйвер сетевого протокола KRON
 */

#include <sys/config.h>
#include "csu/csu.h"
#include "csu/mtd.h"
#include "key/key.h"
#include "lcd/lcd.h"
#include "tsens/ds1820.h"
#include "spi/adc/ads1118.h"
#include "kron.h"
#include "kron_imp.h"

static bool LongTx = false;
static rs_pkt_t Buff; /* буфер приема/передачи */
BUS_STATE KronBusState; /* машина сотояния приема кадра */
static unsigned char IpBuff; /* указатель данных в буфере приема */
unsigned char KronIdleCount; /* счетчик интервалов времени */
static unsigned int preset_I,  preset_Id, preset_U;

static void tx_reply (void);
static void frame_parse (void);

/* драйвер KRON SLAVE устройства */
void kron_drv (unsigned char ip, unsigned char len)
{
    if (KronBusState != BUS_STOP) { /* активен прием кадра */
        if (LongTx) {
            STOP_RX();
            KronBusState = BUS_START;
            return;
        }
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
        if (LongTx) tx_reply();
        else if (IpBuff > KRON_RX_MIN) frame_parse();
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
		if ((rx.dest_adr != Cfg.addr) && (rx.dest_adr != SYS_ADR)
            && (rx.dest_adr != BROAD_ADR)) {
			if ((rx.dest_adr & MULTI_ADR) == 0) return;
			else {
				if ((rx.dest_adr & 0x7F) != (Cfg.addr & 0x60)) return;
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
									if (CsuState != rd.rx_data.cmd) {
										SelfCtrl = 0;
										Start_CSU((csu_st)rd.rx_data.cmd); //если изменилась комнада, то запустить блок с новой командой	
									}
								}
							}
						} else {
							if (((CsuState | RELAY_EN) != rd.rx_data.cmd) || Error)
                                Stop_CSU((csu_st)rd.rx_data.cmd);
						}	
					}
                    break;
                case USER_CFG:
					if (rx.length >= 12) { //если поле данных конфигурирования не пустое
						if (rd.rx_usr.cmd.bit.ADR_SET) {
							if ((rd.rx_usr.Adress!=0)&&(rd.rx_usr.Adress!=0xFF)) 
								Cfg.addr = rd.rx_usr.Adress;	
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
					if (rx.length >= 12) { //если поле данных конфигурирования не пустое
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
						if (rx.length >= 13) //Если есть данные о количестве РМ
							Cfg.dmSlave = rd.rx_sys.dm_cnt;
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
					if (CsuState != 0) Stop_CSU(0);
                    switch (rd.rx_alg.cmd) {
                    case FIND_CMD:
						mCnt = sCnt = cCnt = 0;//номера метода, этапа и цикла
						for (cnt = 0; cnt < MTD_N; cnt++) StgNum[cnt] = 0;
						msNum = find_free();
                        break;
                    case DEL_CMD:
						delete_all_mtd();
						msNum = 0;
                        break;
                    case SAVE_CMD:
						if (msNum < MS_N) {
							save_alg(msNum, &rd.rx_alg.data[0]);
							msNum++;
						}
                        break;
                    case RST_CMD:
                        msNum = 0;
                    }
                    break;
                case EEPR_PKT:
					msNum = 0;
                    break;
                default:
                    /* Если нет поля с типом пакета (пакет нулевой длины),
                    то считать что это запрос данных */
                    rx.type = DATA_PKT;
                }
            }
            if ((rx.dest_adr != BROAD_ADR) && !multi_adr) tx_reply();
        }
    }
}

#define tx  Buff.fld.header
#define td  Buff.fld.data
#define buf Buff.byte
static void tx_reply (void) {
    LongTx = false;
    uint8_t len = 0;
    tx.start = 0x5A;
    tx.dest_adr = 0;
    tx.src_adr = Cfg.addr;
    switch (tx.type) {
    case DATA_PKT:
        if (PWM_status==0) td.tx_data.operation = PWM_status | RELAY_EN; //Если ШИМ остановлен, то добавить сосотяние реле
        else td.tx_data.operation = PWM_status;
        td.tx_data.error = Error;
        if (PWM_status == DISCHARGE) td.tx_data.I = ADC_O[ADC_DI];
        else td.tx_data.I = ADC_O[ADC_MI];
        td.tx_data.U = ADC_O[ADC_MU];
        td.tx_data.Ip = ADC_O[ADC_MUp];
        td.tx_data.t1 = Temp1.word;
        td.tx_data.t2 = Temp2.word;
        len = sizeof(tx_data_type);
        if (Cfg.bf1.IN_DATA == 0) len--;
        else td.tx_data.In_st = (KEY_MASK ^ 0xF8) >> 3;
        if (Cfg.bf1.OUT_DATA == 0) len--;
        else td.tx_data.Out_st = FAN_ST;
        break;
    case USER_CFG:
        td.tx_usr.cmd = ((uint8_t *)&Cfg.bf1)[0];	
        td.tx_usr.new_adr = Cfg.addr;
        td.tx_usr.K_I = Cfg.K_I;
        td.tx_usr.K_U = Cfg.K_U;
        td.tx_usr.K_Ip = Cfg.K_Ip;
        td.tx_usr.K_Id = Cfg.K_Id;
        td.tx_usr.B_I = Cfg.B[ADC_MI];
        td.tx_usr.B_U = Cfg.B[ADC_MU];
        td.tx_usr.B_Ip = Cfg.B[ADC_MUp];
        td.tx_usr.B_Id = Cfg.B[ADC_DI];
        td.tx_usr.D_I=7;
        td.tx_usr.D_U=7;
        td.tx_usr.D_Id=7;
        td.tx_usr.D_Ip=7;
        if (rd.rx_usr.cmd.bit.ADR_SET) tx.src_adr = rx.dest_adr; //подставить другой адрес в ответе, если происходит изменени адреса
        //rx_pack.fld.data.rx_usr.cmd.bit.ADR_SET=0;
        //tx_lenght_calc=sizeof(tx_usr_type)-12; //-12 только для того чтобы работала старая CE1
        len = sizeof(tx_usr_type); 
        break;
    case SYS_CFG:
        td.tx_sys.cmd.byte = ((uint8_t *)&Cfg.bf1)[0];	
        td.tx_sys.mode.byte = ((uint8_t *)&Cfg.bf1)[1];
        td.tx_sys.maxU = Cfg.maxU;
        td.tx_sys.maxI = Cfg.maxI;
        td.tx_sys.maxId = Cfg.maxId;
        td.tx_sys.maxPd = Cfg.P_maxW;
        td.tx_sys.dm_cnt = Cfg.dmSlave;
        td.tx_sys.slave_cnt_u=0;
        td.tx_sys.slave_cnt_i=0;
        td.tx_sys.cfg = Cfg.bf2.autostart;
        td.tx_sys.autostart_try = Cfg.cnt_set;//AUTOSTART_CNT;
        td.tx_sys.restart_timout = Cfg.time_set * 16;
        td.tx_sys.autostart_u = Cfg.u_set;
        len = sizeof(tx_sys_type);
        break;
    case VER_PKT:
        td.tx_ver.hard_ver = HW_VER;
        td.tx_ver.hard_mode = HW_MODE;
        td.tx_ver.soft_ver = SW_VER;
        td.tx_ver.soft_mode = SW_MODE;
        read_num(&td.tx_ver.number[0]);
        len = sizeof(tx_ver_type);
        break;
    case ALG_PKT:
        td.tx_alg.mem_point = MS_SIZE * (MS_N - msNum);
        len = sizeof(tx_alg_type);
        if (msNum < MS_N) td.tx_alg.result = 0;
        else td.tx_alg.result = 1;
        break;
    case EEPR_PKT: // ToDo: next cycle transmit
        eeread_mtd(msNum, (mtd_t *)td.tx_EEPROM.D);
        td.tx_EEPROM.ADR = CFG_SIZE + msNum * MS_SIZE;
        msNum++;
        if (msNum < MS_N) LongTx = true;
        len = MS_SIZE;
	}
    tx.length = len + 2; //прибавить два байта к длине: номер пакета и тип пакета
    len += 6; //прибавить ещё 4 байта заголовка и 1 байт CRC
    buf[len] = calc_crc(buf, len);
    BuffLen = len + 1; /* полный размер буфера при передаче кадра */
    len = buf[0]; /* сохранить первый байт кадра */
    TxIpBuff = 1; /* указатель на начало буфера (второй байт!) */
    start_tx(len, buf); /* стартовать передачу кадра */
    RsActive = true;
}
