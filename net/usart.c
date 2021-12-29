#include <system.h>
#include "csu/csu.h"
#include "csu/mtd.h"
#include "lcd/wh2004.h"
#include "lcd/lcd.h"
#include "ee/eeprom.h"
#include "key/key.h"
#include "pwm/pwm.h"
#include "tsens/ds1820.h"
#include "usart.h"

extern int TEST1, TEST2, TEST3, TEST4;

extern unsigned char tx_point;
extern tx_pack_type tx_pack;
extern unsigned char rx_point;
extern rx_pack_type rx_pack;
extern unsigned char tx_lenght;
extern unsigned char connect_st;
extern unsigned char time_wait;

extern unsigned char addr;
extern unsigned char NEED_TX;

extern autosrart_t autosrart;
extern CSU_type CSU_cfg;
extern CSU2_type CSU_cfg2;
extern unsigned char dmSlave;
extern unsigned char PwmStatus, CsuState, Error;
extern unsigned char SelfCtrl; //управление методом заряда производится самостоятельно или удалённо
extern Temp_type Temp1, Temp2;
extern ADC_Type  ADC_ADS1118[4];
extern unsigned int ADC_O[4]; //данные АЦП без изменений (бе вычета коэфициента В)
extern unsigned int set_I, set_U, set_Id;
extern unsigned int preset_I,  preset_Id, preset_U;
extern unsigned int K_I, K_U, K_Id, K_Ip, max_set_I, max_set_Id, max_set_U, P_maxW;
extern unsigned int maxI, maxId, maxU; 
extern unsigned char change_UI;

extern unsigned int B[4];

extern unsigned char Mtd_cnt, Stg_cnt, cycle_cnt;//номера метода, этапа и цикла
extern unsigned int Mtd_ARD[15];
extern unsigned int Wr_ADR;

//---------------------инициализация usart`a---------------------
void Init_USART(void)
{
  RS485_Rx;  
  UBRRH=0;
  UBRRL=16; //скорость обмена 115200 бод на частоте 16Мгц
  UCSRA=0x22;
  UCSRB=(1<<RXCIE)|((1<<TXCIE))|(1<<RXEN)|(1<<TXEN); //разр. прерыв при приеме, разр приема, разр передачи.
  UCSRC=(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);  //размер слова 8 разрядов
}

//---------------------отправка символа по usart`у---------------------
void Putch(unsigned char sym,  unsigned char lenght)
{
  tx_lenght=lenght;
  while(!(UCSRA & (1<<UDRE)));
  RS485_Tx;
  UDR = sym; 
  tx_point++;
}
/*
 * Прерывание по событию: Receive Complete.
 * Принят байт данных.
 */
#pragma vector=UART(USART,_RXC_vect)
#pragma type_attribute=__interrupt
void usart_rx_byte(void)
{unsigned char ch;

if (UCSRA&0x80) //Если байт принят
	{
	time_wait=2;
	ch = UDR;
	if ((UCSRA&0x10)!=0x10) //Если Frame error равен нулю
		{
		rx_pack.byte[rx_point]=ch; //Сохранить байт в буфере
		rx_point++;	
		if (rx_point>=RX_LENGTH) rx_point=0;
		}		
	}
Rx_Data();	//Проверить нет ли принятого пакета и орбработать*/
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
RS485_Rx;	
if (tx_point>=tx_lenght) 
	{
	tx_point=0;
	}
else 
	{
	Putch(tx_pack.byte[tx_point], tx_lenght);	
	}
}

//---------------------Передача данных на ПК----------------------
void Tx_Data(unsigned char type)
{unsigned char cnt, tx_lenght_calc=0;
tx_pack.fld.header.start=0x5A;
tx_pack.fld.header.dest_adr=0;
tx_pack.fld.header.src_adr=addr;
//tx_pack.fld.header.length=TX_LENGTH-5;
tx_pack.fld.header.number=rx_pack.fld.header.number;
tx_pack.fld.header.type=type;
if (type==1) //Если необходимо отправить пакет с данными
	{
	if (PwmStatus==0) tx_pack.fld.data.tx_data.operation=PwmStatus|RELAY_EN; //Если ШИМ остановлен, то добавить сосотяние реле
	else			   tx_pack.fld.data.tx_data.operation=PwmStatus;
	tx_pack.fld.data.tx_data.error=Error;
	if (PwmStatus==discharge)	tx_pack.fld.data.tx_data.I=ADC_O[ADC_DI];//tx_pack.fld.data.tx_data.I=ADC_ADS1118[ADC_DI].word;
	else						tx_pack.fld.data.tx_data.I=ADC_O[ADC_MI];//tx_pack.fld.data.tx_data.I=ADC_ADS1118[ADC_MI].word;
	tx_pack.fld.data.tx_data.U=ADC_O[ADC_MU];//ADC_ADS1118[ADC_MU].word;
	tx_pack.fld.data.tx_data.Ip=ADC_O[ADC_MUp];//ADC_ADS1118[ADC_MUp].word;
	tx_pack.fld.data.tx_data.t1=Temp1.word;
	tx_pack.fld.data.tx_data.t2=Temp2.word;
	tx_lenght_calc=sizeof(tx_data_type);
	if (Cfg.bf1.IN_DATA==0) tx_lenght_calc--;
	else tx_pack.fld.data.tx_data.In_st=(KEY_MASK^0xF8)>>3;
	if (Cfg.bf1.OUT_DATA==0) tx_lenght_calc--;
	else
		{
		tx_pack.fld.data.tx_data.Out_st=FAN_ST;
		}
	}
if (type==2) //Если необходимо отправить пакет конфигурирования пользовательских даных
	{
	tx_pack.fld.data.tx_usr.cmd=CSU_cfg.byte[0];	
	tx_pack.fld.data.tx_usr.new_adr=addr;
	tx_pack.fld.data.tx_usr.K_I=K_I;
	tx_pack.fld.data.tx_usr.K_U=K_U;
	tx_pack.fld.data.tx_usr.K_Ip=K_Ip;
	tx_pack.fld.data.tx_usr.K_Id=K_Id;
	tx_pack.fld.data.tx_usr.B_I=B[ADC_MI];
	tx_pack.fld.data.tx_usr.B_U=B[ADC_MU];
	tx_pack.fld.data.tx_usr.B_Ip=B[ADC_MUp];
	tx_pack.fld.data.tx_usr.B_Id=B[ADC_DI];
	tx_pack.fld.data.tx_usr.D_I=7;
	tx_pack.fld.data.tx_usr.D_U=7;
	tx_pack.fld.data.tx_usr.D_Id=7;
	tx_pack.fld.data.tx_usr.D_Ip=7;
	if (rx_pack.fld.data.rx_usr.cmd.bit.ADR_SET) tx_pack.fld.header.src_adr=rx_pack.fld.header.dest_adr; //подставить другой адрес в ответе, если происходит изменени адреса
	rx_pack.fld.data.rx_usr.cmd.bit.ADR_SET=0;
	//tx_lenght_calc=sizeof(tx_usr_type)-12; //-12 только для того чтобы работала старая CE1
	tx_lenght_calc=sizeof(tx_usr_type); 
	}
if (type==3) //Если необходимо отправить пакет конфигурирования системных даных
	{
	tx_pack.fld.data.tx_sys.cmd.byte=CSU_cfg.byte[0];	
	tx_pack.fld.data.tx_sys.mode.byte=CSU_cfg.byte[1];
	tx_pack.fld.data.tx_sys.maxU=maxU;
	tx_pack.fld.data.tx_sys.maxI=maxI;
	tx_pack.fld.data.tx_sys.maxId=maxId;
	tx_pack.fld.data.tx_sys.maxPd=P_maxW;
	tx_pack.fld.data.tx_sys.dm_cnt=dmSlave;
	tx_pack.fld.data.tx_sys.slave_cnt_u=0;
	tx_pack.fld.data.tx_sys.slave_cnt_i=0;
	tx_pack.fld.data.tx_sys.cfg=CSU_cfg2.bit.autostart;
	tx_pack.fld.data.tx_sys.autostart_try=autosrart.cnt_set;//AUTOSTART_CNT;
	tx_pack.fld.data.tx_sys.restart_timout=autosrart.time_set*16;
	//tx_pack.fld.data.tx_sys.autostart_u=autosrart.u;
	tx_pack.fld.data.tx_sys.autostart_u=autosrart.u_set;
	tx_lenght_calc=sizeof(tx_sys_type);
	}
if (type==4) //Если необходимо отправить с версией блока
	{
	tx_pack.fld.data.tx_ver.hard_ver = Rev_ver;
	tx_pack.fld.data.tx_ver.hard_mode = Rev_mode;
	tx_pack.fld.data.tx_ver.soft_ver = Soft_ver;
	tx_pack.fld.data.tx_ver.soft_mode = Soft_mod;
	read_num(&tx_pack.fld.data.tx_ver.number[0]);
	tx_lenght_calc=sizeof(tx_ver_type);
	}
if (type==5)
	{
	tx_pack.fld.data.tx_alg.mem_point=EEPROM_SIZE-Wr_ADR;
	tx_lenght_calc=sizeof(tx_alg_type);
	if (Wr_ADR<EEPROM_SIZE) tx_pack.fld.data.tx_alg.result=0;
	else tx_pack.fld.data.tx_alg.result=1;
	}
	
NEED_TX=0;

if (type==6)
	{
	EEPROM_read_string(Wr_ADR, 32, &tx_pack.fld.data.tx_EEPROM.D[0]);
	tx_pack.fld.data.tx_EEPROM.ADR=Wr_ADR;
	Wr_ADR+=32;
	if (Wr_ADR<EEPROM_SIZE)
		{
		NEED_TX=type;
		}
	tx_lenght_calc=sizeof(tx_EEPROM_type);
	time_wait=100;
	}

tx_pack.fld.header.length=tx_lenght_calc+2; //прибавить два байта к длине: номер пакета и тип пакета
tx_lenght_calc=tx_pack.fld.header.length+5; //прибавить ещё 4 байта заголовка и 1 байт CRC
tx_pack.byte[tx_lenght_calc-1]=0;
for (cnt=0; cnt<(tx_lenght_calc-1);cnt++)
	{
	tx_pack.byte[tx_lenght_calc-1]=CRC8_Tmp(tx_pack.byte[cnt], tx_pack.byte[tx_lenght_calc-1]);	
	}

Putch(tx_pack.byte[0], tx_lenght_calc);
}


void Rx_Data(void)
{unsigned char CRC_RX=0, cnt, multi_adr=0;
if (rx_point>4) //если принято больше 4-х байт
	{
	if (rx_pack.fld.header.length<=(rx_point-5)) //если число принятых байт данных равно значению в поле длинны и принято CRC
		{
		rx_point=0;
		time_wait=5;
		if (!Cfg.bf1.PCC_ON) //Если не установлен флаг управления с ПК
			{
			if (rx_pack.fld.header.type==1) return; //если это пакет даных, то не обрабатывает его если не установлен флаг управления с ПК
			if ((rx_pack.fld.header.dest_adr!=SYS_ADR)&&(rx_pack.fld.header.type==2)) return; //если это запрос пользовательской конфигурации не с системного адреса
			}
		
		if ((rx_pack.fld.header.dest_adr!=addr)&&(rx_pack.fld.header.dest_adr!=SYS_ADR)&&(rx_pack.fld.header.dest_adr!=BROAD_ADR))  //Если на совпадает адрес получателя, то проверить мультикастовый пакет или нет
			{
			if ((rx_pack.fld.header.dest_adr&MULTI_ADR)==0) return;
			else
				{
				if ((rx_pack.fld.header.dest_adr&0x7F)!=(addr&0x60)) return;
				else multi_adr=1;
				//if ((rx_pack.fld.header.dest_adr|addr)!=rx_pack.fld.header.dest_adr) return;
				//else multi_adr=1;
				}
			}
		
		if ((rx_pack.fld.header.start==0xA5)&&(rx_pack.fld.header.src_adr==0x00)) //если принят верный пакет
			{
			for (cnt=0; cnt<(rx_pack.fld.header.length+4); cnt++)
				{
				CRC_RX=CRC8_Tmp(rx_pack.byte[cnt], CRC_RX);	
				}
			if (CRC_RX!=rx_pack.byte[rx_pack.fld.header.length+4]) return;//*/
			
			if (rx_pack.fld.header.length>1) //если в пакете есть поле типа пакета
				{
				if (rx_pack.fld.header.type==0x01) //Если приянт пакет с данными
					{	
					if (rx_pack.fld.header.length>5)	//если в пакете есть информация о задаваемом токе
						{
						if ((rx_pack.fld.data.rx_data.cmd&0x0F)==charge) //если режим "заряд"
							{
							if (set_I!=rx_pack.fld.data.rx_data.setI) change_UI=1;
							if (rx_pack.fld.data.rx_data.setI>B[ADC_MI])
								preset_I=rx_pack.fld.data.rx_data.setI-B[ADC_MI];
							else preset_I=0;
							if (preset_I>max_set_I) preset_I=max_set_I;
							//set_I=rx_pack.fld.data.rx_data.setI;	
							//if (set_I>max_set_I) set_I=max_set_I;
							}
						if ((rx_pack.fld.data.rx_data.cmd&0x0F)==discharge) //если режим "разряд"
							{
							if (set_Id!=rx_pack.fld.data.rx_data.setI) change_UI=1;
							if (rx_pack.fld.data.rx_data.setI>B[ADC_DI])
								preset_Id=rx_pack.fld.data.rx_data.setI-B[ADC_DI];
							else preset_Id=0;
							if (preset_Id>max_set_Id) preset_Id=max_set_Id;
							//set_Id=rx_pack.fld.data.rx_data.setI;
							//if (set_Id>max_set_Id) set_Id=max_set_Id;
							}
						}//if (rx_pack.fld.length>3)
					if (rx_pack.fld.header.length>7)						//в пакете есть данные о задаваемом напряжении?
						{
						if (set_U!=rx_pack.fld.data.rx_data.setU) change_UI=1;
						if (rx_pack.fld.data.rx_data.setU>B[ADC_MU])
							preset_U=rx_pack.fld.data.rx_data.setU-B[ADC_MU];
						else preset_U=0;
						if (preset_U>max_set_U) preset_U=max_set_U;
						//set_U=rx_pack.fld.data.rx_data.setU;
						//if (set_U>max_set_U) set_U=max_set_U;
						}//if (rx_pack.fld.length>5)
					if (rx_pack.fld.header.length>3) //если в пакете есть контрольные биты
						{
						if (Cfg.bf1.FAN_CONTROL) //Если разрешён контроль выходных реле и вентиляторов
							{
							if (rx_pack.fld.data.rx_data.control.bit.FAN1_ON) FAN(1);
							else FAN(0);
							}
						}
					if (rx_pack.fld.header.length>2) //если в пакете данных есть команда
						{
						if ((rx_pack.fld.data.rx_data.cmd&0x0F)!=0) //Если это команда запуска разряда или заряда
							{
							if ((rx_pack.fld.data.rx_data.cmd&0xF0)==0) //Если пришла команда со сброшенным флагом отсрочки выполнения (0x0x: 0x01 или 0x02)
								{
								if (rx_pack.fld.data.rx_data.cmd==0x03)
									{
									//autosrart.restart_cnt=AUTOSTART_CNT;
									key_power();
									}
								else
									{
									set_I=preset_I; //установить зарядный ток
									set_Id=preset_Id; //установить разрядный ток
									set_U=preset_U; //установить напряжение
									if (CsuState!=rx_pack.fld.data.rx_data.cmd) 
										{
										SelfCtrl=0;
										csu_start(rx_pack.fld.data.rx_data.cmd); //если изменилась комнада, то запустить блок с новой командой	
										}
									}
								}
							}
						else
							{
							if (((CsuState|RELAY_EN)!=rx_pack.fld.data.rx_data.cmd)||(Error!=0)) csu_stop(rx_pack.fld.data.rx_data.cmd);
							}	
						}//	if (rx_pack.pack.header.length>2) //если в пакете данных есть команда					
					}//if (rx_pack.fld.type==0x01)
					
				if (rx_pack.fld.header.type==0x02) //Если принят пакет конфигурирования данных пользователя
					{				
					if (rx_pack.fld.header.length>=12)//если поле данных конфигурирования не пустое
						{
						if (rx_pack.fld.data.rx_usr.cmd.bit.ADR_SET) 
							{
							if ((rx_pack.fld.data.rx_usr.Adress!=0)&&(rx_pack.fld.data.rx_usr.Adress!=0xFF)) 
								addr=rx_pack.fld.data.rx_usr.Adress;	
							}
						K_I=rx_pack.fld.data.rx_usr.K_I;
						K_U=rx_pack.fld.data.rx_usr.K_U;
						K_Ip=rx_pack.fld.data.rx_usr.K_Ip;
						K_Id=rx_pack.fld.data.rx_usr.K_Id;
						if (rx_pack.fld.header.length>=20)
							{				
							B[ADC_MI]=rx_pack.fld.data.rx_usr.B_I;		
							B[ADC_MU]=rx_pack.fld.data.rx_usr.B_U;
							B[ADC_MUp]=rx_pack.fld.data.rx_usr.B_Ip;
							B[ADC_DI]=rx_pack.fld.data.rx_usr.B_Id;
							}
	
						calc_cfg();		
						if (rx_pack.fld.data.rx_usr.cmd.bit.EEPROM) EEPROM_save_cfg();
						}
					}
					
				if (rx_pack.fld.header.type==0x03) //Если принят пакет конфигурирования системных даных
					{
					if (rx_pack.fld.header.length>=0x0C)//если поле данных конфигурирования не пустое
						{
						rx_pack.fld.data.rx_sys.mode.bit.RELAY_MODE=1;
						rx_pack.fld.data.rx_sys.cmd.bit.TE_DATA=0;	
						CSU_cfg.byte[0]=rx_pack.fld.data.rx_sys.cmd.byte;
						CSU_cfg.byte[1]=rx_pack.fld.data.rx_sys.mode.byte;
						if (Cfg.bf1.LCD_ON) Cfg.bf1.LED_ON=0; //Защита от одновременного включения LCD и LED (у LCD Приоритет)
						maxU=rx_pack.fld.data.rx_sys.maxU;
						maxI=rx_pack.fld.data.rx_sys.maxI;
						maxId=rx_pack.fld.data.rx_sys.maxId;
						P_maxW=rx_pack.fld.data.rx_sys.maxPd;
						if (rx_pack.fld.header.length>=0x0D) //Если есть данные о количестве РМ
							{
							dmSlave=rx_pack.fld.data.rx_sys.dm_cnt;
							}
						if (rx_pack.fld.header.length>=17) //если есть поле дополнительной конфигурации
							{
							CSU_cfg2.word=rx_pack.fld.data.rx_sys.cfg;	
							}
						if (rx_pack.fld.header.length>=22) //если есть информация об автозапуске
							{
							autosrart.cnt_set=rx_pack.fld.data.rx_sys.autostart_try;
							autosrart.u_set=rx_pack.fld.data.rx_sys.autostart_u;
							autosrart.time_set=rx_pack.fld.data.rx_sys.restart_timout/16;
							}
						calc_cfg();
						if (rx_pack.fld.data.rx_sys.cmd.bit.EEPROM) EEPROM_save_cfg();
						}
					}		
							
				if (rx_pack.fld.header.type==0x04) //Если принят пакет с версией ПО
					{
					if (rx_pack.fld.header.length==0x0C)//если поле данных конфигурирования не пустое
						{	
						if (rx_pack.fld.data.rx_ver.cmd.bit.EEPROM!=0)
							save_num(&rx_pack.fld.data.rx_ver.number[0]);
						if (Cfg.bf1.LCD_ON) lcd_wr_connect(1);
						}						
					}

				if (rx_pack.fld.header.type==0x05) //Если принят пакет с алгоритмом программы
					{
					if (CsuState!=0) csu_stop(0);
					if (rx_pack.fld.data.rx_alg.cmd==0x01) 
						{	
						Mtd_cnt=0; Stg_cnt=0; cycle_cnt=0;//номера метода, этапа и цикла
						for (cnt=1; cnt<15; cnt++) Mtd_ARD[cnt]=0;
						Mtd_ARD[0]=Mtd_START_ADR;
						Wr_ADR=find_free_memory(Mtd_cnt);
						}
					if (rx_pack.fld.data.rx_alg.cmd==0x20)
						{
						delete_all_Mtd();
						Wr_ADR=Mtd_START_ADR;
						}
					if (rx_pack.fld.data.rx_alg.cmd==0x03)
						{
						if (Wr_ADR<EEPROM_SIZE)
							{
							EEPROM_write_string(Wr_ADR, rx_pack.fld.data.rx_alg.size, &rx_pack.fld.data.rx_alg.data[0]);
							Wr_ADR+=rx_pack.fld.data.rx_alg.size;
							}
						}
					if (rx_pack.fld.data.rx_alg.cmd==0x04)
						{
						Wr_ADR=Mtd_START_ADR;
						}
					}
				if (rx_pack.fld.header.type==0x06) //Если принят пакет с запросом EEPROM
					{
					Wr_ADR=Mtd_START_ADR;
					}					
				}//if (rx_pack.fld.length>1)	
			else rx_pack.fld.header.type=1; //Если нет поля с типом пакета (пакет нулевой длины), то считать что это запрос данных					
			
			connect_st=120;	//установить 120*16 мс таймаут для ожижания приёма следующего пакета
			
			//if (rx_pack.fld.header.type==0x11) rx_pack.fld.header.type=0;

			if ((rx_pack.fld.header.dest_adr!=BROAD_ADR)&&(multi_adr==0)) NEED_TX=rx_pack.fld.header.type;
			else NEED_TX=0;
			}
		
		}//if (rx_pack.fld.length<=(rx_point-5)) 
	}//if (rx_point>4)

}