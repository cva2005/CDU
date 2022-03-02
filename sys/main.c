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
    ALARM_OUT(OFF);
    read_cfg();
    lcd_start();
    init_rs();
    SYS_TMR_ON();
    START_RX();
    __enable_interrupt();
    adc_init();
    // ToDo: use WDT?
    if (Cfg.cmd.fan_cntrl == 0) {
        FAN(ON);
        delay_ms(500);
        delay_ms(500);
        read_mtd();
        FAN(OFF);
    }
    while (true) {	  	
        adc_drv();
        csu_drv();
        net_drv();
    }
}

static inline void init_gpio (void) { // ToDo: delete magic numbers!
    DDRA = 0x03;
    DDRB = 0x18;
#if !JTAG_DBGU
    DDRC = 0xFF;
    PORTC = 0xFF;
#endif
    DDRD = 0xF4;
    MCUCR = (1 << ISC00) | (1 << ISC10);
    GICR = 1 << INT1; // ext int
}