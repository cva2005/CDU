#pragma message	("@(#)dcon.c     1.00    09/09/22 OWEN")

/*
 * Драйвер сетевого протокола DCON
 */

#include <sys/system.h>
//#include <sys/config.h>
//#include <net/dsc.h>
//#include <meas/meas.h>
//#include <sys/floatErr.h>
#include "kron.h"
#include "kron_imp.h"

static void frame_parse(void);
static unsigned char calc_crc(unsigned char *buf, unsigned char len);
static bool str_to_int(char *uint, char *str);
static void int_to_str(char uint, char *str);
static unsigned char char_to_int(char inp_char);
static char int_to_char(char uint);
static void float_to_str(float flo, char *str, char i);

static unsigned char KronBuff[KRON_BUFF_LEN]; /* буфер приема/передачи */
BUS_STATE KronBusState; /* машина сотояния према кадра */
static unsigned char KronIpBuff; /* указатель данных в буфере приема */

/* драйвер DCON SLAVE устройства */
void kron_drv(unsigned char ip, unsigned char len)
{
    while (len--) {
        if (ip >= RX_BUFF_LEN) ip = 0;
        unsigned char tmp = RxBuff[ip++];
        if (tmp == DC_IO_DATA) { /* обнаружен стартовый байт DCON */
            KronIpBuff = 0; /* указатель на начало буфера */
            KronBusState = BUS_START; /* первый байт кадра принят */
        }
        if (KronBusState == BUS_START) { /* первый байт кадра уже принят */
            if (tmp == DC_STOP) { /* принят стоп байт DCON */
                if (KronIpBuff >= KRON_RX_MIN) { /* длина кадра в норме */
                    KronBusState = BUS_STOP; /* завершить прием */
                    frame_parse();
                    KronBusState = BUS_IDLE; /* шину в режим ожидания */
                } else { /* длина кадра мала */
                    KronBusState = BUS_IDLE; /* шину в режим ожидания */
                }
            } else if (KronIpBuff <= KRON_BUFF_LEN) { /* буфер не переполнен */
                KronBuff[KronIpBuff++] = tmp; /* записать байт в буфер */
            } else { /* переполнение буфера */
                KronBusState = BUS_IDLE; /* шину в режим ожидания */
            }
        }
    }
}

/* разбор кадра DCON */
static void frame_parse(void)
{
    unsigned char i;

    if (str_to_int((char *)&i, (char *)(KronBuff + 1)) &&
        (i == (/*Cfg.Addres &*/ 0xff))) { /* адрес совпал */
        if (str_to_int((char *)&i, (char *)(KronBuff + KronIpBuff - 2)) &&
            (i == calc_crc(KronBuff, KronIpBuff - 2))) { /* CRC совпала */
            if (KronIpBuff == ONE_CH) { /* поканальное считывание данных */
                i = char_to_int(KronBuff[3]); /* сохранить номер канала */
                if (i == ERR_CHAR) return;
                if (i < INP_NUM) { /* номер канала валидный */
                    KronBuff[0] = DC_DATA_OK;
                    float_to_str(ch_data[i].value, (char *)&DconBuff[1], i);
                    BuffLen = CH_DATA_LEN + 1; /* размер без CRC */
                } else { /* запрос несуществующего входа */
                    KronBuff[0] = DC_COMM_ERR;
                    BuffLen = 3; /* размер буфера без CRC */
                }
            } else if (KronIpBuff == ALL_CH) { /* групповое считывание данных */
                KronBuff[0] = DC_DATA_OK;
                for (i = 0; i < INP_NUM; i++) { /* запись данных каналов */
                    float_to_str(ch_data[i].value,
                        (char *)&DconBuff[1 + i * CH_DATA_LEN], i);
                }
                BuffLen = (CH_DATA_LEN * INP_NUM) + 1; /* размер без CRC */
            } else return; /* команда не поддерживается */
            i = calc_crc(KronBuff, BuffLen);
            int_to_str(i, (char *)(KronBuff + BuffLen));
            BuffLen += 2; /* размер буфера с CRC */
            KronBuff[BuffLen++] = DC_STOP; /* добавить стоп байт */
            TxIpBuff = 1; /* указатель на начало буфера (второй байт!) */
            start_tx(KronBuff[0], KronBuff); /* стартовать передачу кадра */
        }
    }
}

/* Вычисление контрольной суммы кадра DCON */
static unsigned char calc_crc(unsigned char *buf, unsigned char len)
{
    unsigned char crc = 0;

    while (len--) crc += buf[len];
    return crc;
}

/*
 * Перевод числа из строки в формат unsigned char.
 * При обнаружении ошибки возвращается значение FALSE.
 */
static bool str_to_int(char *uint, char *str)
{
    unsigned char dig;

    dig = char_to_int(str[0]);
    if (dig != ERR_CHAR) {
        *uint = dig << 4;
        dig = char_to_int(str[1]);
        if (dig != ERR_CHAR) {
            *uint |= dig;
            return true;
        }
    }
    return false;
}

/* Перевод числа из формат unsigned char в строку */
static void int_to_str(char uint, char *str)
{
    str[0] = int_to_char(uint >> 4);
    str[1] = int_to_char(uint);
}

/*
 * Перевод символа в формат unsigned char.
 * При обнаружении ошибки функция возвращает значение ERR_CHAR.
 */
static unsigned char char_to_int(char inp_char)
{
    if ((inp_char >= '0') && (inp_char <= '9')) return (inp_char - 0x30);
    else if ((inp_char >= 'A') && (inp_char <= 'F')) return (inp_char - 0x37);
    else return ERR_CHAR; /* недопустимый символ */
}

/* Перевода шестнадцатиричного числа в символ */
static char int_to_char(char uint)
{
    uint &= 0x0f;
    if (uint <= 9) return (uint + 0x30);
    return (uint + 0x37);
}

const float error_val[] = { /* значения при сообщении об ошибке */
     -9999.9, /* 0 */
     -9999.9, /* 1 */
     -9999.9, /* 2 */
     -9999.9, /* 3 */
     -9999.9, /* 4 */
     -9999.9, /* 5 */
     -9999.9, /* 6 данные не готовы */
     -9999.9, /* 7 отключен датчик */
     +9999.9, /* 8 велика температура CJC */
     -9999.9, /* 9  мала  температура CJC */
     +9999.9, /* 10 значение велико */
     -9999.9, /* 11 значение мало */
     -9999.9, /* 12 короткое замыкание */
     +9999.9, /* 13 обрыв датчика */
     +9999.9, /* 14 нет связи */
     -9999.9, /* 15 не тот калибровочный коэфициент */
};
/*
 * Перевод значения индекированного float числа в строку.
 * Длинна каждой записи числа равна 7 символам (включая знак и точку).
 * При передаче значений менее 10 вначале значения добавляется 0.
 * Десятичная точка может быть смешена не более чем на 2 знака!
 */
static void float_to_str(float flo, char *str, char i)
{
    unsigned char ecode;
    char dp;

    ecode = *((unsigned char *)&(flo) + 3);
    if (ecode > emErrValue) { /* значение содержит ошибку */
        flo = error_val[ecode - emErrValue];
        dp = 1;
    } else {
        dp = Cfg.Dp[i]; /* положение десятичной точки */
    }
    if (flo < 0) {
        str[0] = '-'; /* записать в буфер знак "-" */
        flo *= - 1;
    } else str[0] = '+'; /* записать в буфер знак "+" */
    for (i = 0; i < dp; i++) flo *= 10.0; /* число с учетом точности */
    flo += 0.5; /* округление числа */
    i = CHAR_NUM + 1; /* символов в строковом представлении числа */
    do {
        if (dp != CHAR_NUM + 1 - i) {
            str[i] = (char)((unsigned long)flo % 10) + '0';
            flo /= 10.0;
        } else  str[i] = '.'; /* записать в буфер знак "." */
    } while (--i);
}
