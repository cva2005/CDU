#pragma message	("@(#)dcon.c     1.00    09/09/22 OWEN")

/*
 * Драйвер сетевого протокола DCON
 */

#include <sys/system.h>
#include "kron.h"
#include "kron_imp.h"

static void frame_parse(void);
static uint8_t calc_crc(uint8_t *buf, uint8_t len);

static unsigned char KronBuff[KRON_BUFF_LEN]; /* буфер приема/передачи */
BUS_STATE KronBusState; /* машина сотояния приема кадра */
static unsigned char KronIpBuff; /* указатель данных в буфере приема */
unsigned char KronIdleCount; /* счетчик интервалов времени */

/* драйвер KRON SLAVE устройства */
void kron_drv(unsigned char ip, unsigned char len)
{
    while (len--) {
        if (ip >= RX_BUFF_LEN) ip = 0;
        unsigned char tmp = RxBuff[ip++];
        if (tmp == CHAR_START) { /* обнаружен стартовый байт KRON */
            KronIpBuff = 0; /* указатель на начало буфера */
            KronBusState = BUS_START; /* первый байт кадра принят */
        }
        if (KronBusState == BUS_START) { /* первый байт кадра уже принят */
             if (KronIpBuff <= KRON_BUFF_LEN) { /* буфер не переполнен */
                KronBuff[KronIpBuff++] = tmp; /* записать байт в буфер */
            } else { /* переполнение буфера */
                KronBusState = BUS_IDLE; /* шину в режим ожидания */
            }
        } else if (KronBusState == BUS_STOP) {  /* завершен прием кадра */
            if (KronIpBuff >= KRON_RX_MIN) { /* длина кадра в норме */
                KronBusState = BUS_STOP; /* завершить прием */
                frame_parse();
                KronBusState = BUS_IDLE; /* шину в режим ожидания */
            } else { /* длина кадра мала */
                KronBusState = BUS_IDLE; /* шину в режим ожидания */
            }
        }
    }
}

/* разбор кадра KRON */
static void frame_parse(void)
{
#if 0
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
#endif
}
/* Вычисление контрольной суммы кадра KRON */
static uint8_t calc_crc(uint8_t *buf, uint8_t len)
{
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
