#pragma message	("@(#)net.c")

#include <sys/system.h>
#include "net.h"
#include "net_imp.h"

static bool RsActive = false;
static bool RsBusy = false;
stime_t TimeIdle;
uint8_t *BuffPtr; /* указатель на буфер передачи */
uint8_t TxIpBuff; /* указатель данных в буфере передачи */
uint8_t BuffLen; /* размер буфера при передаче кадра */
uint8_t RxBuff[RX_BUFF_LEN]; /* кольцевой буфер приема */
int8_t RxIpNew; /* указатель хвоста буфера приема */
int8_t RxIpOld; /* указатель головы буфера приема */
static NET_FUNC *net_func[] = { /* сетевые функции */
    rtu_drv,
    ascii_drv,
    kron_drv
};

#define NET_FUNC_NUM (sizeof(net_func) / sizeof(*net_func))

/* Настройка драйвера RS */
void init_rs (void)
{
    RS_DIR_INIT();
    UART(UCSR,C) = SHL(UART(URSEL,)) | SHL(UART(UCSZ,1)) | SHL(UART(UCSZ,0)); // Data Bits: 8
    //UART(UCSR,C) &= ~(SHL(UART(UPM,0)) | SHL(UART(UPM,1))); // Parity: None
    //UART(UCSR,C) &= ~SHL(UART(USBS,)); // Stop Bits: 1
    SET_BAUD(115200);
    UART(UCSR,B) |= SHL(UART(TXEN,));
}

/* Сетевой драйвер верхнего уровня */
void net_drv(void)
{
    if (!get_time_left(TimeIdle)) RsActive = false;
    uint8_t new_ip = RxIpNew;
    uint8_t old_ip = RxIpOld;
    uint8_t len;
    if (new_ip < old_ip) len = (RX_BUFF_LEN - old_ip) + new_ip;
    else len = new_ip - old_ip;
    RxIpOld = new_ip;
    for (uint8_t i = 0; i < NET_FUNC_NUM; i++) {
        if (TX_ACTIVE()) return;
        net_func[i](old_ip, len);
    }
}

/* Сетевой драйвер нижнего уровня */

/* Иницирует начало передачи кадра */
void start_tx(char first, uint8_t *buff)
{
    STOP_RX(); /* запретить прием */
    BuffPtr = buff; /* сохранить указатель на буфер передачи */
    RS485_OUT(); /* линию управления RS485 - на передачу */
    delay_us(20);
    /* разрешить передачу и прерывание TX UART */
    UART(UCSR,B) |= SHL(UART(UDRIE,));
    UART(UDR,) = first; /* загрузить первый байт */
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
        UART(UCSR,A) |= SHL(UART(TXC,));   /* сбросим возможный флаг TXC */
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
    RsBusy = false;
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
    if (UART(UCSR,A) & ERROR_BITS) {
        START_RX(); /* начать прием заново */
    } else { /* аппаратных ошибок не обнаружено */
        RX_TMR_ON(); /* запустить таймер тайм-аута приема кадра */
        if (RxIpNew >= RX_BUFF_LEN) RxIpNew = 0;
        RxBuff[RxIpNew++] = UART(UDR,); /* сохранить принятый байт */
    }
}

/*
 * Прерывание по событию:
 * Срабатывание таймера тайм-аута
 * при преме кадра
 */
#pragma vector=RX_OCR_IRQ
#pragma type_attribute=__interrupt
void rx_tick_irq(void)
{
    if (RtuBusState > BUS_STOP) {
        if (RtuIdleCount == FRAME_RTU_SYNC) { /* межкадровый интервал выдержан */
            RtuBusState = BUS_STOP; /* завершить прием кадра */
            RsBusy = true;
        } else RtuIdleCount++; /* скорректировать интервал "молчания" */
    }
    if (KronBusState > BUS_STOP) {
        if (KronIdleCount == FRAME_KRON_SYNC) { /* межсимв. интервал превышен */
            KronBusState = BUS_STOP; /* завершить прием кадра */
            RsBusy = true;
        } else KronIdleCount++; /* скорректировать интервал "молчания" */
    }
    if (AsciiBusState > BUS_STOP) {
        if (AsciiIdleCount == FRAME_ASCII_ERROR) { /* межсимв. интервал превышен */
            AsciiBusState = BUS_IDLE; /* шину в режим ожидания */
            RsBusy = true;
        } else AsciiIdleCount++; /* скорректировать интервал "молчания" */
    }
}

bool rs_active (void) {
    return RsActive;
}

bool rs_busy (void) {
    return RsBusy;
}

void rs_set_busy (void) {
    RsBusy = true;
}

void set_active (void) {
    TimeIdle = get_fin_time(SEC(2));
    RsActive = true;
}