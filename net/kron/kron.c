#pragma message	("@(#)kron.c")

/*
 * Драйвер сетевого протокола KRON
 */

#include <sys/config.h>
#include "key/key.h"
#include "lcd/lcd.h"
#include "pwm/pwm.h"
#include "tsens/ds1820.h"
#include "csu/csu.h"
#include "csu/mtd.h"
#include "kron.h"
#include "kron_imp.h"

static bool LongTx = false;
static rs_pkt_t Buff; /* буфер приема/передачи */
BUS_STATE KronBusState; /* машина сотояния приема кадра */
static uint8_t IpBuff; /* указатель данных в буфере приема */
uint8_t KronIdleCount; /* счетчик интервалов времени */
static uint16_t preTaskI,  preTaskId, preTaskU;

static void tx_reply (void);
static void frame_parse (void);

/* драйвер KRON SLAVE устройства */
void kron_drv (uint8_t ip, uint8_t len)
{
    if (KronBusState != BUS_STOP) { /* активен прием кадра */
        if (LongTx) {
            STOP_RX();
            KronBusState = BUS_START;
            return;
        }
        while (len--) {
            if (ip >= RX_BUFF_LEN) ip = 0;
            uint8_t tmp = RxBuff[ip++];
            if (tmp == CHAR_RS && KronBusState != BUS_START) { /* обнаружен стартовый байт KRON */
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
		if (!Cfg.mode.pcc) { //Если не установлен флаг управления с ПК
			/* если это пакет даных, то не обрабатывает его если не установлен флаг управления с ПК */
            if (rx.type == 1) return;
			/* если это запрос пользовательской конфигурации не с системного адреса */
            if ((rx.dest_adr != SYS_ADR) && (rx.type == 2)) return;
		}
        /* Если на совпадает адрес получателя, то проверить мультикастовый пакет или нет */
		if (rx.dest_adr != Cfg.addr && rx.dest_adr != SYS_ADR
            && rx.dest_adr != BROAD_ADR) {
			if ((rx.dest_adr & MULTI_ADR) == 0) return;
			else {
				if ((rx.dest_adr & 0x7F) != (Cfg.addr & 0x60)) return;
				else multi_adr = true;
			}
		}
		if (rx.start == CHAR_RS && !rx.src_adr) { //если принят верный пакет
            if (calc_crc(Buff.byte, rx.length + 4) != Buff.byte[rx.length + 4]) return;
			if (rx.length > 1) { //если в пакете есть поле типа пакета
                csu_st rcmd = (csu_st)(rd.rdata.cmd & 0x0F);
                switch (rx.type) {
                case DATA_PKT:
                    if (rx.length > 2) { //если в пакете данных есть команда
                        if (rx.length > 3) { //если в пакете есть контрольные биты
                            if (Cfg.cmd.fan_cntrl) { //Если разрешён контроль выходных реле и вентиляторов
                                if (rd.rdata.fcntl.fan1) FAN(ON);
                                else FAN(OFF);
                            }
                            Cfg.bf2.bit.pulse = rd.rdata.fcntl.pulse;
                            if (rx.length > 5) { //если в пакете есть информация о задаваемом токе
                                if (rcmd == CHARGE) { //если режим "заряд"
                                    if (rd.rdata.setI > Cfg.B[ADC_MI])
                                        preTaskI = rd.rdata.setI - Cfg.B[ADC_MI];
                                    else preTaskI = 0;
                                    if (preTaskI > MaxI) preTaskI = MaxI;
                                } else if (rcmd == DISCHARGE) { //если режим "разряд"
                                    if (rd.rdata.setI > Cfg.B[ADC_DI])
                                        preTaskId = rd.rdata.setI - Cfg.B[ADC_DI];
                                    else preTaskId = 0;
                                    if (preTaskId > MaxId) preTaskId = MaxId;
                                }
                                if (rx.length > 7) { //в пакете есть данные о задаваемом напряжении?
                                    if (rd.rdata.setU > Cfg.B[ADC_MU])
                                        preTaskU = rd.rdata.setU - Cfg.B[ADC_MU];
                                    else preTaskU = 0;
                                    if (preTaskU > MaxU) preTaskU = MaxU;
                                }
                            }
                        }
                        if (rcmd) { // команда запуска разряда или заряда
                            if (!(rd.rdata.cmd & 0xF0)) { //Если пришла команда со сброшенным флагом отсрочки выполнения (0x0x: 0x01 или 0x02)
                                if (rcmd == PULSE) {
                                    key_power();
                                } else {
                                    set_task_ic(preTaskI); //установить зарядный ток
                                    set_task_id(preTaskId); //установить разрядный ток
                                    set_task_u(preTaskU); //установить напряжение
                                    //если изменилась комнада, то запустить блок с новой командой
                                    if (CsuState != rcmd) {
                                        SelfCtrl = false;
                                        csu_start(rcmd);
                                    }
                                }
                            }
                        } else {
                            if (((CsuState | IS_RELAY_EN()) != rd.rdata.cmd) || get_csu_err()) {
                                csu_stop(rcmd);
                            }
                        }	
                    }
                    break;
                case USER_CFG:
                    if (rx.length >= 12) { //если поле данных конфигурирования не пустое
                        if (rd.usr.cmd.addr_set) {
                            if (rd.usr.addr && rd.usr.addr != SYS_ADR) 
                                Cfg.addr = rd.usr.addr;	
                        }
                        Cfg.K_I = rd.usr.K_I;
                        Cfg.K_U = rd.usr.K_U;
                        Cfg.K_Ip = rd.usr.K_Ip;
                        Cfg.K_Id = rd.usr.K_Id;
                        if (rx.length >= 20) {				
                            Cfg.B[ADC_MI] = rd.usr.B_I;		
                            Cfg.B[ADC_MU] = rd.usr.B_U;
                            Cfg.B[ADC_MUp] = rd.usr.B_Ip;
                            Cfg.B[ADC_DI] = rd.usr.B_Id;
                        }
                        calc_cfg();		
                        if (rd.usr.cmd.eepr) save_cfg();
                    }
                    break;
                case SYS_CFG:
                    if (rx.length >= 12) { //если поле данных конфигурирования не пустое
                        rd.sys.mode.reley = ON;
                        rd.sys.cmd.te_data = OFF;	
                        Cfg.cmd = rd.sys.cmd;
                        Cfg.mode = rd.sys.mode;
                        /* Защита от одновременного включения LCD и LED (LCD Приоритет) */
                        if (Cfg.mode.lcd) Cfg.mode.led = OFF;
                        Cfg.maxU = rd.sys.maxU;
                        Cfg.maxI = rd.sys.maxI;
                        Cfg.maxId = rd.sys.maxId;
                        Cfg.P_maxW = rd.sys.maxPd;
                        if (rx.length >= 13) { // данные о количестве РМ
                            Cfg.dmSlave = rd.sys.dm_cnt;
                            if (rx.length >= 17) { //  поле дополнительной конфигурации
                                Cfg.bf2.word = rd.sys.cfg;
                                if (rx.length >= 22) { //если есть информация об автозапуске
                                    Cfg.cnt_set = rd.sys.autostart_try;
                                    Cfg.u_set = rd.sys.autostart_u;
                                    Cfg.time_set = rd.sys.restart_timout;
                                }
                            }
                        }
                        calc_cfg();
                        if (rd.sys.cmd.eepr) save_cfg();
                    }
                    break;
                case VER_PKT:
                    if (rx.length == 0x0C) { //если поле данных конфигурирования не пустое
                        if (rd.rver.cmd.eepr)
                            save_num(&rd.rver.num[0]);
                        if (Cfg.mode.lcd) lcd_wr_connect(true);
                    }						
                    break;
                case ALG_PKT:
                    if (CsuState) csu_stop(STOP);
                    switch (rd.algr.cmd) {
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
                            save_alg(msNum, &rd.algr.data[0]);
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
    uint8_t len =  tx.dest_adr = 0;
    tx.start = CHAR_TS;
    tx.src_adr = Cfg.addr;
    csu_st pwm_st = pwm_state();
    switch (tx.type) {
    case DATA_PKT:
        /* Если ШИМ остановлен, то добавить сосотяние реле */
        if (pwm_st == STOP) pwm_st |= IS_RELAY_EN();
        td.tdata.oper = pwm_st;
        td.tdata.error = get_csu_err();
        if (pwm_state() == DISCHARGE) td.tdata.I = ADC_O[ADC_DI];
        else td.tdata.I = ADC_O[ADC_MI];
        td.tdata.U = ADC_O[ADC_MU];
        td.tdata.Ip = ADC_O[ADC_MUp];
        td.tdata.t1 = get_csu_t(TCH0);
        td.tdata.t2 = get_csu_t(TCH1);
        len = sizeof(tdata_t);
        if (!Cfg.cmd.in_data) len--;
        else td.tdata.In_st = (KEY_MASK ^ 0xF8) >> 3;
        if (!Cfg.cmd.out_data) len--;
        else td.tdata.Out_st = FAN_ST;
        break;
    case USER_CFG:
        td.usr.cmd = Cfg.cmd;	
        td.usr.addr = Cfg.addr;
        td.usr.K_I = Cfg.K_I;
        td.usr.K_U = Cfg.K_U;
        td.usr.K_Ip = Cfg.K_Ip;
        td.usr.K_Id = Cfg.K_Id;
        td.usr.B_I = Cfg.B[ADC_MI];
        td.usr.B_U = Cfg.B[ADC_MU];
        td.usr.B_Ip = Cfg.B[ADC_MUp];
        td.usr.B_Id = Cfg.B[ADC_DI];
        td.usr.D_I = td.usr.D_U = td.usr.D_Id = td.usr.D_Ip = 7;
        /* адрес в ответе, если происходит изменени адреса */
        if (rd.usr.cmd.addr_set) tx.src_adr = rx.dest_adr;
        len = sizeof(usr_t); 
        break;
    case SYS_CFG:
        td.sys.cmd = Cfg.cmd;	
        td.sys.mode = Cfg.mode;
        td.sys.maxU = Cfg.maxU;
        td.sys.maxI = Cfg.maxI;
        td.sys.maxId = Cfg.maxId;
        td.sys.maxPd = Cfg.P_maxW;
        td.sys.dm_cnt = Cfg.dmSlave;
        td.sys.slave_cnt_u = td.sys.slave_cnt_i = 0;
        td.sys.cfg = Cfg.bf2.bit.astart;
        td.sys.autostart_try = Cfg.cnt_set;//AST_CNT;
        td.sys.restart_timout = Cfg.time_set;
        td.sys.autostart_u = Cfg.u_set;
        len = sizeof(sys_t);
        break;
    case VER_PKT:
        td.tver.hard_ver = HW_VER;
        td.tver.hard_mode = HW_MODE;
        td.tver.soft_ver = SW_VER;
        td.tver.soft_mode = SW_MODE;
        read_num(&td.tver.number[0]);
        len = sizeof(tver_t);
        break;
    case ALG_PKT:
        td.algt.pmem = MS_SIZE * (MS_N - msNum);
        len = sizeof(algt_t);
        if (msNum < MS_N) td.algt.res = 0;
        else td.algt.res = 1;
        break;
    case EEPR_PKT: // ToDo: next cycle transmit
        eeread_mtd(msNum, (mtd_t *)td.eepr.d);
        td.eepr.adr = CFG_SIZE + msNum * MS_SIZE;
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
    set_active();
}
