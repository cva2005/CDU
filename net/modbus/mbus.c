#pragma message	("@(#)mbus.c     1.00    09/09/22 OWEN")

/*
 * Драйвер сетевого протокола MODBUS
 */

#include <sys/system.h>
//#include <sys/config.h>
//#include <sys/floatErr.h>
//#include <meas/meas.h>
//#include <net/dsc.h>
#include "mbus.h"
#include "mbus_imp.h"

static void frame_parse(BUS_MODE mode);
static unsigned short rtu_crc(unsigned char *buf, unsigned char len);
static unsigned char ascii_lcr(unsigned char *buf, unsigned char len);
static bool read_reg(unsigned char *buf, unsigned short first,
    unsigned short len);

static unsigned char RtuBuff[RTU_BUFF_LEN]; /* буфер приема/передачи RTU */
static unsigned char AsciiBuff[ASCII_BUFF_LEN]; /* буфер приема/передачи ASCII */
BUS_STATE RtuBusState; /* машина сотояния према кадра RTU */
BUS_STATE AsciiBusState; /* машина сотояния према кадра ASCII */
static unsigned char RtuIpBuff; /* указатель данных в буфере приема */
static unsigned char AsciiIpBuff; /* указатель данных в буфере приема */
unsigned char RtuIdleCount; /* счетчик интервалов времени RTU */
unsigned char AsciiIdleCount; /* счетчик интервалов времени ASCII */

/* Драйвер MODBUS ASCII */
void ascii_drv(unsigned char ip, unsigned char len)
{
    while (len--) {
        if (ip >= RX_BUFF_LEN) ip = 0;
        unsigned char tmp = RxBuff[ip++];
        if (tmp == MB_START) { /* принят маркер "старт" */
            AsciiIpBuff = 0; /* указатель на начало буфера */
            AsciiBusState = BUS_START; /* маркер "старт" принят */
        } else if (AsciiBusState != BUS_IDLE) { /* шина не в режиме ожидания */
            if (AsciiBusState == BUS_START) { /* маркер "старт" уже принят */
                if (tmp == MB_STOP_1) { /* принят первый стоп байт */
                    AsciiBusState = BUS_FULL;
                } else if (AsciiIpBuff <= ASCII_BUFF_LEN) { /* запись в буфер */
                    if ((tmp >= 0x30) && (tmp <= 0x39)) {
                        AsciiBuff[AsciiIpBuff++] = tmp - 0x30;
                    } else if ((tmp >= 0x41) && (tmp <= 0x46)) {
                        AsciiBuff[AsciiIpBuff++] = tmp - 0x37;
                    } else { /* принят недопустимый символ */
                        AsciiBusState = BUS_IDLE; /* шину в режим ожидания */
                    }
                } else { /* переполнение буфера */
                    AsciiBusState = BUS_IDLE; /* шину в режим ожидания */
                }
            } else if (tmp == MB_STOP_2) { /* принят второй стоп */
                if ((AsciiBusState == BUS_FULL) && /* первый стоп уже принят */
                    (AsciiIpBuff >= MB_ASCII_RX_MIN) && /* размер в норме */
                    (!(AsciiIpBuff % 2))) { /* четность кадра в норме */
                    AsciiBusState = BUS_STOP; /* завершить прием кадра */
                    frame_parse(MODE_ASCII); /* на разбор кадра */
                    AsciiBusState = BUS_IDLE; /* шину в режим ожидания */
                } else {
                    AsciiBusState = BUS_IDLE; /* шину в режим ожидания */
                }
            } else if (AsciiBusState == BUS_FULL) { /* первый стоп принят */
                /* не пришел второй стоп после первого! */
                AsciiBusState = BUS_IDLE; /* шину в режим ожидания */
            }
        }
    }
}

/* Драйвер MODBUS RTU */
void rtu_drv(unsigned char ip, unsigned char len)
{
    if (RtuBusState != BUS_STOP) { /* активен прием кадра */
        if (RtuBusState == BUS_IDLE) {
            /* принят первый байт кадра MODBUS RTU */
            RtuIpBuff = 0; /* указатель на начало буфера */
            RtuBusState = BUS_START; /* первый байт кадра принят */
        }
        while (len--) {
            if (ip >= RX_BUFF_LEN) ip = 0;
            if (RtuIpBuff <= RTU_BUFF_LEN) { /* запись в буфер */
                RtuBuff[RtuIpBuff++] = RxBuff[ip++]; /* записать байт */
            } else { /* переполнение буфера */
                RtuBusState = BUS_IDLE; /* шину в режим ожидания */
                break;
            }
        }
    } else { /* RtuBusState == BUS_STOP */
        if (RtuIpBuff >= MB_RTU_RX_MIN) { /* длина кадра в норме */
            frame_parse(MODE_RTU);
        }
        RtuBusState = BUS_IDLE; /* шину в режим ожидания */
    }
}

/* Драйвер MODBUS */
static void frame_parse(BUS_MODE mode)
{
    unsigned char i, j; /* счетчики */
    MODBUS_ERROR mb_error = NO_MB_ERROR; /* регистр ошибок MODBUS */
    unsigned char *rs_buff; /* указатель на буфер приема/передачи */
    unsigned short raddr; /* начальный адрес регистра MODBUS */
    unsigned short rnum; /* количество регистров MODBUS */

    if (mode == MODE_ASCII) {
        i = 0; /* счетчик длинного буфера */
        j = 0; /* счетчик короткого буфера */
        for (; i < AsciiIpBuff; j++) { /* склеивание тетрад в буфере */
            AsciiBuff[j] = AsciiBuff[i++] << 4; /* запись старшей тетрады */
            AsciiBuff[j] |= AsciiBuff[i++]; /* запись младшей тетрады */
        }
        AsciiIpBuff /= 2; /* размер декодированного кадра */
        rs_buff = AsciiBuff;
    } else { /* mode == MODE_RTU */
        rs_buff = RtuBuff;
    }
    if (rs_buff[0] != MB_COMMON) { /* не широковещательный адрес */
        if (rs_buff[0] != (MY_ADR)) {
            return; /* адрес не совпал */
        }
    } else { /* широковещательный адрес */
        return; /* не поддерживается при чтении! */
    }
    if (mode == MODE_ASCII) {
        if (rs_buff[AsciiIpBuff - 1] !=
            ascii_lcr(&rs_buff[0], AsciiIpBuff - 1))
            return; /* ошибка LCR */
    } else { /* mode == MODE_RTU */
        if (*((unsigned short *)&rs_buff[RtuIpBuff - 2]) !=
            rtu_crc(&rs_buff[0], RtuIpBuff - 2))
            return; /* ошибка CRC */
    }
    if ((rs_buff[1] == RD_HOLD_REG) || (rs_buff[1] == RD_INP_REG)) {
        raddr = ((unsigned short)rs_buff[2] << 8);
        raddr += rs_buff[3]; /* начальный адрес регистра MODBUS */
        rnum = ((unsigned short)rs_buff[4] << 8);
        rnum += rs_buff[5]; /* количество регистров для чтения */
        if (read_reg(&rs_buff[3], raddr, rnum) != false) {
            BuffLen = rs_buff[2] = (unsigned char)rnum * 2; /* счетчик байт */
            BuffLen += 3; /* размер буфера без контрольной суммы */
        } else { /* обнаружена ошибка */
            mb_error = ILLEGAL_DATA_ADDRESS;
        }
    } else { /* запрошенная функция не поддерживается */
        mb_error = ILLEGAL_FUNCTION;
    }
    if (mb_error != NO_MB_ERROR) { /* генерация сообщения об ошибке */
        rs_buff[1] |= ERROR_MASK; /* признак ошибки в поле кода функции */
        rs_buff[2] = mb_error; /* код ошибки */
        BuffLen = 3; /* размер буфера без контрольной суммы */
    }
    if (mode == MODE_ASCII) {
        *((char *)&rs_buff[BuffLen]) = ascii_lcr(&rs_buff[0], BuffLen);
        BuffLen++; /* размер некодированного буфера с LCR */
        j = BuffLen;
        BuffLen *= 2; /* размер кодированного буфера с LCR */
        i = BuffLen - 1; /* указатель хвоста длинного буфера */
        do { /* формирование тетрад в буфере */
            rs_buff[i--] = (rs_buff[--j] & 0x0f);
            rs_buff[i--] = (rs_buff[j] >> 4);
        } while (j);
        for (i = 0; i < BuffLen; i++) { /* кодирование символов */
            j = rs_buff[i];
            if (j < 0x0a) j += 0x30;
            else j += 0x37;
            rs_buff[i] = j;
        }
        rs_buff[BuffLen++] = MB_STOP_1; /* маркер стоп 1 */
        rs_buff[BuffLen++] = MB_STOP_2; /* маркер стоп 2 */
        i = MB_START; /* сохранить первый байт кадра */
        TxIpBuff = 0; /* указатель на начало буфера */
    } else { /* mode == MODE_RTU */
        *((short *)&rs_buff[BuffLen]) = rtu_crc(rs_buff, BuffLen);
        BuffLen += 2; /* полный размер буфера при передаче кадра */
        i = rs_buff[0]; /* сохранить первый байт кадра */
        TxIpBuff = 1; /* указатель на начало буфера (второй байт!) */
    }
    start_tx(i, rs_buff); /* стартовать передачу кадра */
    return;
}

/* заполнение буфера содержимым регистров */
static bool read_reg(unsigned char *buf, unsigned short first,
                     unsigned short len)
{
    if ((len == 0)  || ((first + len) > REG_NUM)) return false;
    len += first;
    for (; (char)first < (char)len; first++) {
        signed short int_val;
        float flo_val;
        unsigned char i;
        unsigned char idx = first / REG_IDX_NUM; /* индекс группы */
        switch ((char)(first % REG_IDX_NUM)) { /* номер параметра в группе */
        case DPOINT_POS:
            *buf++ = 0;
            *buf++ = /*Cfg.Dp[idx]*/0;
            continue;
        case MEAS_INT:
            flo_val = /*ch_data[idx].value*/0;
            i = /*Cfg.Dp[idx]*/0;
            while (i--) flo_val *= 10;
            int_val = (signed short)flo_val;
            break;
        case MEAS_STAT:
            i = *((unsigned char *)/*&ch_data[idx].value +*/ 3);
            if (i <= /*emErrValue*/1) { /* значение не содержит ошибку */
                int_val = 0x0000;
            } else { /* ошибка измерения*/
                int_val = 0xf000 + (i & 0x0f);
            }
            break;
        case MEAS_TIME:
            int_val = /*ch_data[idx].time*/0;
            break;
        case MEAS_FLOATL:
            int_val = *((short *)/*&ch_data[idx].value*/0);
            break;
        default: /* MEAS_FLOATH */
            int_val = *((short *)/*&ch_data[idx].value +*/ 1);
        }
        *buf++ = int_val >> 8;
        *buf++ = (unsigned char)int_val;
    }
    return true;
}

/* вычисление контрольной суммы кадра MODBUS RTU CRC */
static unsigned short rtu_crc(unsigned char *buf, unsigned char len)
{
    unsigned char bit_cnt;
    unsigned char buf_cnt;
    unsigned short crc = 0xffff;
    bool poly_xor;

    for (buf_cnt = 0; buf_cnt < len; buf_cnt++) {
        crc ^= *(buf + buf_cnt);
        for (bit_cnt = 0; bit_cnt < 8; bit_cnt++) {
            if (crc & 0x0001) poly_xor = true;
            else poly_xor = false;
            crc >>= 1;
            if (poly_xor)  crc ^= 0xa001;
        }
    }
    return crc;
}

/* Вычисление контрольной суммы кадра MODBUS ASCII LCR */
static unsigned char ascii_lcr(unsigned char *buf, unsigned char len)
{
    unsigned char lrc = 0;

    while (len--) lrc += buf[len];
    return -lrc;
}