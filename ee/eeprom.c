#include "eeprom.h"
#include "csu/csu.h"
#include "csu/mtd.h"
#include "net/usart.h"
#include "tsens/ds1820.h"

extern unsigned int K_I, K_U, K_Id, K_Ip;//, K_speed;
extern unsigned int B[4];
extern unsigned int B_I[3];
extern unsigned int maxI, maxId, maxU;
extern unsigned int P_maxW;
extern CSU_type CSU_cfg;
extern CSU2_type CSU_cfg2;
extern unsigned char dmSlave;
extern unsigned char addr;
extern autosrart_t autosrart;

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
/* Wait for completion of previous write */
while(EECR & (1<<EEWE));
/* Set up Address and Data Registers */
EEAR = uiAddress;
EEDR = ucData;
/* Write logical one to EEMWE */
EECR |= (1<<EEMWE);
/* Start eeprom write by setting EEWE */
EECR |= (1<<EEWE);
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
/* Wait for completion of previous write */
while(EECR & (1<<EEWE));
/* Set up Address Register */
EEAR = uiAddress;
/* Start eeprom read by writing EERE */
EECR |= (1<<EERE);
/* Return data from Data Register */
return EEDR;
}

void EEPROM_write_int(unsigned int uiAddress, unsigned int ucData)
{
	EEPROM_write(uiAddress, ucData);
	EEPROM_write(uiAddress+1, (ucData>>8));	
}

unsigned int EEPROM_read_int(unsigned int uiAddress)
{unsigned int temp_data=0;
	temp_data=EEPROM_read(uiAddress+1);
	temp_data<<=8;
	temp_data|=EEPROM_read(uiAddress);
	return (temp_data);
}

void EEPROM_save_cfg(void)
{unsigned char CRC=0, i;
	EEPROM_write_int(0, K_U);
	EEPROM_write_int(2, K_I);
	EEPROM_write_int(4, K_Id);
	EEPROM_write_int(6, K_Ip);
	EEPROM_write_int(8, maxU);
	EEPROM_write_int(10, maxI);
	EEPROM_write_int(12, maxId);
	EEPROM_write_int(14, P_maxW);
	EEPROM_write_int(16, CSU_cfg.word);
	EEPROM_write(18, dmSlave);
	EEPROM_write(19, addr);
	EEPROM_write_int(20, B[ADC_MU]);
	EEPROM_write_int(22, B[ADC_MI]);
	EEPROM_write_int(24, B[ADC_DI]);
	EEPROM_write_int(26, B[ADC_MUp]);
	EEPROM_write_int(28, CSU_cfg2.word); 
	EEPROM_write_int(30,autosrart.time_set);
	EEPROM_write_int(32, autosrart.u_set);
	EEPROM_write(34, autosrart.cnt_set);
	EEPROM_write(35, 0);
	
	for (i=0; i<LAST_EEPROM_CFG; i++) CRC=CRC8_Tmp(EEPROM_read(i), CRC);	
	EEPROM_write(LAST_EEPROM_CFG, CRC);
}
	
	
void EEPROM_read_cfg(void)
{unsigned char CRC=0, i;
	for (i=0; i<LAST_EEPROM_CFG; i++) CRC=CRC8_Tmp(EEPROM_read(i), CRC);	
	if (CRC==EEPROM_read(LAST_EEPROM_CFG))
		{
		K_U=EEPROM_read_int(0);
		K_I=EEPROM_read_int(2);
		K_Id=EEPROM_read_int(4);
		K_Ip=EEPROM_read_int(6);
		maxU=EEPROM_read_int(8);
		maxI=EEPROM_read_int(10);
		maxId=EEPROM_read_int(12);
		P_maxW=EEPROM_read_int(14);
		CSU_cfg.word=EEPROM_read_int(16);
		dmSlave=EEPROM_read(18);
		addr=EEPROM_read(19);
		B[ADC_MU]=EEPROM_read_int(20);
		B[ADC_MI]=EEPROM_read_int(22);
		B[ADC_DI]=EEPROM_read_int(24);
		B[ADC_MUp]=EEPROM_read_int(26);
		CSU_cfg2.word=EEPROM_read_int(28);
		autosrart.time_set=EEPROM_read_int(30);
		autosrart.u_set=EEPROM_read_int(32);
		autosrart.cnt_set=EEPROM_read(34);
		}		
	else
		{
		K_U=K_U_const;
		K_I=K_I_const;
		K_Id=K_Id_const;
		K_Ip=K_Up_const;
		
		B[ADC_MU]=B_U_const;
		B[ADC_MI]=B_I[dmSlave]; //смещение тока установить в зависимости от того сколько РМ подключено
		B[ADC_DI]=B_Id_const;
		B[ADC_MUp]=B_Up_const;
		
		CSU_cfg2.bit.autostart=AUTOSTART;
		
		autosrart.cnt_set=AUTOSTART_CNT;
		autosrart.time_set=AUTOSTART_TIME;
		autosrart.u_set=AUTOSTART_U;
		
		maxI=maxI_const;
		maxU=maxU_const;
		maxId=maxId_const;
		P_maxW=maxPd_const;
		
		addr=addr_const;
		dmSlave=DM_ext;
		
		CSU_cfg.word=0;
		//CSU_cfg.bit.EEPROM=1;
		//CSU_cfg.bit.ADR_SET=1;
		CSU_cfg.bit.IN_DATA=IN_DATA_bit;
		CSU_cfg.bit.OUT_DATA=OUT_DATA_bit;
		//CSU_cfg.bit.TE_DATA=TE_DATA_bit;
		//CSU_cfg.bit.FAN_CONTROL=1;
		CSU_cfg.bit.DIAG_WIDE=DIAG_WIDE_bit; 
		CSU_cfg.bit.I0_SENSE=I0_SENSE_bit;
		CSU_cfg.bit.LCD_ON=LCD_ON_bit;
		CSU_cfg.bit.LED_ON=LED_ON_bit;
		CSU_cfg.bit.PCC_ON=PCC_ON_bit;
		CSU_cfg.bit.DEBUG_ON=DEBUG_ON_bit;
		CSU_cfg.bit.GroupM=GroupM_bit;
		CSU_cfg.bit.EXT_Id=EXT_Id_bit;
		//CSU_cfg.bit.EXTt_pol=1;
		CSU_cfg.bit.RELAY_MODE=RELAY_MODE_bit;
		}
	calc_cfg();
	}
	
void EEPROM_save_number(unsigned char *point)
{unsigned int i;
 unsigned char CRC=0;
	for (i=0; i<8; i++)
		{
		EEPROM_write(i+START_EEPROM_NUMBER,point[i]);
		CRC=CRC8_Tmp(point[i], CRC);
		}
	EEPROM_write(8+START_EEPROM_NUMBER,CRC);
}
	
void EEPROM_read_number(char *point)
{unsigned int i; 
unsigned char CRC=0;
for (i=0; i<8; i++)
	{
	point[i]=EEPROM_read(i+START_EEPROM_NUMBER);
	CRC=CRC8_Tmp(point[i], CRC);
	}
if (CRC!=EEPROM_read(8+START_EEPROM_NUMBER))
	{
	for (i=0; i<8; i++) point[i]=0xFF;	
	}
}

unsigned char EEPROM_read_string(unsigned int Mtd_adr, unsigned char size, unsigned char *str)
{unsigned char cnt, CRC=0;

for (cnt=0; cnt<size; cnt++) 
	{
	str[cnt]=EEPROM_read(cnt+Mtd_adr);
	if (cnt<(size-1)) CRC=CRC8_Tmp(str[cnt], CRC);
	}
if (CRC!=str[size-1]) return(0);
return(1);

}

void EEPROM_write_string(unsigned int Mtd_adr, unsigned char size, unsigned char *str)
{unsigned char cnt, CRC=0;

for (cnt=0; cnt<size; cnt++)
	{
	if (cnt<(size-1)) CRC=CRC8_Tmp(str[cnt], CRC);
	else str[cnt]=CRC;
	EEPROM_write(cnt+Mtd_adr, str[cnt]);
	}
}