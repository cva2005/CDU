//#define START_EEPROM_CFG 0
#define LAST_EEPROM_CFG 36//последний байт конфигурации (куда записан CRC)
#define START_EEPROM_NUMBER 50
#define LAST_EEPROM_NUMBER  59
#define FIRST_EEPROM_Clb 60
#define Mtd_START_ADR 96 //область расположения методов заряда
#define EEPROM_SIZE 1024

void EEPROM_write(unsigned int uiAddress, unsigned char ucData);
void EEPROM_write_int(unsigned int uiAddress, unsigned int ucData);
unsigned char EEPROM_read(unsigned int uiAddress);
unsigned int EEPROM_read_int(unsigned int uiAddress);
//void EEPROM_save_cfg(void);
//void EEPROM_read_cfg(void);
void EEPROM_save_number(unsigned char *point);
void EEPROM_read_number(char *point);
unsigned char EEPROM_read_string(unsigned int Mtd_adr, unsigned char size, unsigned char *str);
void EEPROM_write_string(unsigned int Mtd_adr, unsigned char size, unsigned char *str);

extern unsigned char addr;
