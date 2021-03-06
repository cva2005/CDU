#pragma message	("@(#)ds1820.c")
#include <system.h>
#include "ds1820.h"

static void read_byte(uint8_t *dat) {
    for (uint8_t i = 0; i < 8; i++) {
        DDR_18B20 |= T1_PIN | T2_PIN;
        delay_us(2);
        DDR_18B20 &= ~(T1_PIN | T2_PIN);
        delay_us(4);
        dat[0] = dat[0] >> 1;
        dat[1] = dat[1] >> 1;
        if (PIN_18B20 & T1_PIN) dat[0] |= 0x80;
        if (PIN_18B20 & T2_PIN) dat[1] |= 0x80;
        delay_us(62);
    }
 }

static void write_byte (uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        DDR_18B20 |= (T1_PIN | T2_PIN);
        delay_us(2);
        if (data & 0x01)
            DDR_18B20 &= ~(T1_PIN | T2_PIN);
        else
            DDR_18B20 |= (T1_PIN | T2_PIN);
        data = data >> 1;
        delay_us(62);
        DDR_18B20 &= ~(T1_PIN | T2_PIN);
        delay_us(2);
    }
}

static uint8_t start_sensor (void) {
    PORT_18B20 &= ~(T1_PIN | T2_PIN);
    DDR_18B20 |= (T1_PIN | T2_PIN);
    delay_us(480);
    DDR_18B20 &= ~(T1_PIN | T2_PIN);
    delay_us(40);
    return PIN_18B20 & (T1_PIN | T2_PIN);
}

#ifdef T_LONG_READ
uint8_t get_tmp_res (int16_t *tmp) {
    uint8_t i, err, d[D_LEN][T_N];
    if (!(err = start_sensor())) {
        delay_us(422);
        write_byte(0xCC); // skip S/N
        write_byte(0x44); // temp convert
        delay_us(1);
        if (!(err = start_sensor())) {
            delay_us(422);
            write_byte(0xCC); // Skip ROM
            write_byte(0xBE); // Read RAM
            for (i = 0; i < D_LEN; i++) read_byte(d[i]);
            for (i = 0; i < T_N; i++) {
                uint8_t k, j, crc = 0;
                for (k = 0; k < 8; k++) {
                    crc = crc ^ d[k][i];
                    for (j = 0; j < 8; j++) {
                        if (crc & 0x01)
                            crc = (crc >> 1) ^ 0x8C;
                        else crc >>= 1;
                    }
                }
                if (d[8][i] == crc) {
                    tmp[i] = d[0][i];
                    tmp[i] |= d[1][i] << 8;
                } else {
                    err |= T_ERROR << i;
                }
            }
        }
    }
    return err;
}
#else
static uint8_t Err, d[D_LEN][T_N], Cnt = 0;
static tstate_t Tstate = START;

tstate_t tmp_drv (void) {
    switch (Tstate) {
    case START:
        Err = start_sensor();
        break;
    case DELAY:
        delay_us(422);
        break;
    case WRITE_1:
        write_byte(0xCC);
        break;
    case WRITE_2:
        if (!Cnt) {
            write_byte(0x44); // temp convert
            Cnt++;
            goto start;
        } else {
            write_byte(0xBE); // Read RAM
            Cnt = 0;
        }
        break;
    case READ:
        read_byte(d[Cnt]);
        if (++Cnt < D_LEN) return READ;
        break;
    default:
        return COMPL;
    }
    Tstate++;
    if (Err) {
        Err = 0;
        Cnt = 0;
    start:
        Tstate = START;
    }
    return Tstate;
}

uint8_t get_tmp_res (int16_t *tmp) {
    if (!Err) {
        for (uint8_t i = 0; i < T_N; i++) {
            uint8_t k, j, crc = 0;
            for (k = 0; k < 8; k++) {
                crc = crc ^ d[k][i];
                for (j = 0; j < 8; j++) {
                    if (crc & 0x01)
                        crc = (crc >> 1) ^ 0x8C;
                        else crc >>= 1;
                }
            }
            if (d[8][i] == crc) {
                tmp[i] = d[0][i];
                tmp[i] |= d[1][i] << 8;
            } else {
                Err |= T_ERROR << i;
            }
        }
    }
    Tstate = START;
    Cnt = 0;
    uint8_t err = Err;
    Err = 0;
    return err;
}
#endif