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

static uint8_t crc_calc (uint8_t *pd) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < 8; i++) {
        crc = crc ^ pd[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x01)
                crc = (crc >> 1) ^ 0x8C;
            else
                crc >>= 1;
        }
    }
    return crc;
}

static uint8_t start_sensor (void) {
    PORT_18B20 &= ~(T1_PIN | T2_PIN);
    DDR_18B20 |= (T1_PIN | T2_PIN);
    delay_us(480);
    DDR_18B20 &= ~(T1_PIN | T2_PIN);
    delay_us(40);
    return PIN_18B20 & (T1_PIN | T2_PIN);
}

uint8_t get_tmp_res (int16_t *tmp) {
    uint8_t i, err, d[9][T_N];
    if (!(err = start_sensor())) {
        delay_us(422);
        write_byte(0xCC); // skip S/N
        write_byte(0x44); // temp convert
        delay_ms(1000);
        if (!(err = start_sensor())) {
            delay_us(422);
            write_byte(0xCC); // Skip ROM
            write_byte(0xBE); // Read RAM
            for (i = 0; i < 9; i++) read_byte(d[i]);
            for (i = 0; i < T_N; i++) {
                if (d[8][i] == crc_calc(d[i])) {
                    tmp[i] = *(int16_t *)d[i];
                } else {
                    err |= T_ERROR << i;
                }
            }
        }
    }
    return err;
}