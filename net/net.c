#pragma message	("@(#)net.c     1.00    09/08/12 OWEN")

/*
 * Драйвер RS485
 */

//#include <sys/config.h>
#include <sys/system.h>
#include "net.h"
#include "net_imp.h"

unsigned char *BuffPtr; /* указатель на буфер передачи */
unsigned char TxIpBuff; /* указатель данных в буфере передачи */
unsigned char BuffLen; /* размер буфера при передаче кадра */
unsigned char RsError; /* регистр ошибки сети RS232/RS485 */
static char RsTemp; /* регистр временного хранения */
static char Tmp; /* вспомогательная переменная */
static bool RxError; /* признак сетевой ошибки */
unsigned char RxBuff[RX_BUFF_LEN]; /* кольцевой буфер приема */
signed char RxIpNew; /* указатель хвоста буфера приема */
signed char RxIpOld; /* указатель головы буфера приема */
static NET_FUNC *net_func[] = { /* сетевые функции */
    rtu_drv,
    ascii_drv,
    kron_drv
};

#define NET_FUNC_NUM (sizeof(net_func) / sizeof(*net_func))

/* Настройка драйвера RS */
void init_rs(void)
{
    RS_DIR_INIT();
    UART(UCSR,C) = /*SHL(UART(URSEL,)) |*/ SHL(UART(UCSZ,1)) | SHL(UART(UCSZ,0)); // Data Bits: 8
    //UART(UCSR,C) &= ~(SHL(UART(UPM,0)) | SHL(UART(UPM,1))); // Parity: None
    //UART(UCSR,C) &= ~SHL(UART(USBS,)); // Stop Bits: 1
    SET_BAUD(115200);
    UART(UCSR,B) |= SHL(UART(TXEN,));
}

/* Сетевой драйвер верхнего уровня */
void net_drv(void)
{
    unsigned char new_ip = RxIpNew;
    unsigned char old_ip = RxIpOld;
    unsigned char len;
    if (new_ip < old_ip) len = (RX_BUFF_LEN - old_ip) + new_ip;
    else len = new_ip - old_ip;

    RxIpOld = new_ip;
    for (unsigned char i = 0; i < NET_FUNC_NUM; i++) {
        if (TX_ACTIVE()) return;
        net_func[i](old_ip, len);
    }
}

/* Сетевой драйвер нижнего уровня */

/* Иницирует начало передачи кадра */
void start_tx(char first, unsigned char *buff)
{
    STOP_RX(); /* запретить прием */
    UART(UDR,) = first; /* загрузить первый байт */
    BuffPtr = buff; /* сохранить указатель на буфер передачи */
    RS485_OUT(); /* линию управления RS485 - на передачу */
    /* разрешить передачу и прерывание TX UART */
    UART(UCSR,B) |= (/*SHL(UART(TXEN,)) |*/ SHL(UART(UDRIE,)));
    net_dbprintf("start_tx: Done!\r\n");
}

/*
 * Прерывание по событию: Data Register Empty.
 * Опустошение регистра данных передатчика.
 * Предыдущий байт передан в регистр сдвига передатчика.
 */
#pragma vector=UART(USART,_UDRE_vect)
#pragma type_attribute=__interrupt
void usart_tx_byte(void)
{
    if (TxIpBuff == BuffLen) { /* последний байт кадра регистре сдвига */
        CLR_BIT(UART(UCSR,B), UART(UDRIE,)); /* запреть прерывание UDRE */
        UART(UCSR,A) = SHL(UART(TXC,));   /* сбросим возможный флаг TXC */
        SET_BIT(UART(UCSR,B), UART(TXCIE,)); /* прер. по перед. байта в линию */
    } else { /* передача кадра продолжается */
        UART(UDR,) = BuffPtr[TxIpBuff++]; /* передать очередной байт */
    }
}

/*
 * Прерывание по событию: Transmit Complete.
 * Опустошению сдвигового регистра.
 * Передача кадра завершена, байт
 * полностью передан в физическую линию.
 */
#pragma vector=UART(USART,_TXC_vect)
#pragma type_attribute=__interrupt
void usart_tx_empty(void)
{
    CLR_BIT(UART(UCSR,B), UART(TXCIE,)); /* запр. прер. Data Register Empty */
    START_RX(); /* возобновить прием */
}

/*
 * Прерывание по событию: Receive Complete.
 * Принят байт данных.
 */
#pragma vector=UART(USART,_RXC_vect)
#pragma type_attribute=__interrupt
void usart_rx_byte(void)
{
    Tmp = UART(UCSR,A); /* сохранить регистр статуса */
    RsTemp = UART(UDR,); /* сохранить принятый байт */
    if (Tmp & (SHL(UART(FE,)) | SHL(UART(DOR,)))) { /* апп. ошибка FERR/OERR */
        RsError = 0x21; /* установить регистр ошибки сети RS485 */
        RxError = true; /* установить флаг ошибки */
    } else if (Tmp & SHL(UART(PE,))) { /* аппаратная ошибка паритета */
        RsError = 0x22 /*+ Cfg.RsDataBits*/; /* ошибка паритета */
        RxError = true; /* установить флаг ошибки */
    } else { /* аппаратных ошибок не обнаружено */
        RX_TMR_ON(); /* запустить таймер тайм-аута приема кадра */
        RxError = false; /* сбросить флаг ошибки */
        if (RxIpNew >= RX_BUFF_LEN) RxIpNew = 0;
        RxBuff[RxIpNew++] = RsTemp;
    }
    if (RxError) { /* аппаратная ошибка при приеме */
        START_RX(); /* начать прием заново */
    }
}

/*
 * Прерывание по событию:
 * Срабатывание таймера тайм-аута
 * при преме кадра MODBUS RTU.
 */
#pragma vector=RX_OCR_IRQ
#pragma type_attribute=__interrupt
void rx_tick_irq(void)
{
    if (RtuBusState > BUS_STOP) {
        if (RtuIdleCount == FRAME_RTU_SYNC) { /* межкадровый интервал выдержан */
            RtuBusState = BUS_STOP; /* завершить прием кадра */
        } else RtuIdleCount++; /* скорректировать интервал "молчания" */
    }
    if (KronBusState > BUS_STOP) {
        if (KronIdleCount == FRAME_KRON_ERROR) { /* межсимв. интервал превышен */
            KronBusState = BUS_STOP; /* завершить прием кадра */
        } else KronIdleCount++; /* скорректировать интервал "молчания" */
    }
    if (AsciiBusState > BUS_STOP) {
        if (AsciiIdleCount == FRAME_ASCII_ERROR) { /* межсимв. интервал превышен */
            AsciiBusState = BUS_IDLE; /* шину в режим ожидания */
        } else AsciiIdleCount++; /* скорректировать интервал "молчания" */
    }
}
