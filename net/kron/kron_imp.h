#ifndef KRON_IMP_H
#define KRON_IMP_H

#pragma message	("@(#)kron_imp.h     1.00    09/09/22 OWEN")

/*
 * Драйвер сетевого протокола KRON
 */

#ifdef	__cplusplus
extern "C" {
#endif

/* длина кадра при запросе данных главным (с CRC) */
#define ONE_CH              6 /* поканальное считывание данных */
#define ALL_CH              5 /* групповое считывание данных */
#define KRON_RX_MIN         ALL_CH /* минимальная длина кадра DCON */

#define CHAR_NUM            5 /* символов в строковом представлении числа */
#define CH_DATA_LEN     (CHAR_NUM + 2) /* символов в данных одного канала */
#define ERR_CHAR         0xff /* значение ошибки при преобразовании символа */

#define DC_IO_DATA        '#' /* начало кадра при приеме/запросе данных */
#define DC_DATA_OK        '>' /* начало ответа при успешной обработке данных */
#define DC_COMM_ERR       '?' /* начало ответа при ошибке параметров команды */
#define DC_STOP          0x0d /* символ окончания кадра */
#define DC_ADD_LEN          4 /* общая длина спецсимволов старт/стоп/CRC */

/* размер буфера приема/передачи драйвера KRON */
#define KRON_BUFF_LEN       50

#ifdef __cplusplus
}
#endif

#endif /* KRON_IMP_H */
