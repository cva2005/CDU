#pragma message	("@(#)main.c")
#include <system.h>
#include "net/net.h"
#include "csu/csu.h"
#include "csu/mtd.h"
#include "tsens/ds1820.h"
#include "lcd/lcd.h"
#include "spi/adc/ads1118.h"

static inline void init_gpio (void);

void main (void) {
    init_gpio();
    ALARM_OUT(0);
    read_cfg();
    lcd_start();
    init_rs();
    SYS_TMR_ON();
    START_RX();
    __enable_interrupt();
    adc_init();
    // ToDo: use WDT?
    if (Cfg.bf1.FAN_CONTROL == 0) {
        FAN(1);
        delay_ms(500);
        delay_ms(500);
        read_mtd();
        FAN(0);
    }
    while (true) {	  	
        net_drv();
        adc_drv();
        csu_drv();
    }
}

static inline void init_gpio (void) {
    DDRA = 0x03;
    DDRB = 0xB8;
#if !JTAG_DBGU
    DDRC = 0xFF;
    PORTC = 0xFF;
#endif
    DDRD = 0xF4;
    MCUCR = (1 << ISC00) | (1 << ISC10);
    GICR = 1 << INT1; // ext int
}