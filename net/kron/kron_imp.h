#ifndef KRON_IMP_H
#define KRON_IMP_H
#pragma message	("@(#)kron_imp.h")

/*
 * Драйвер сетевого протокола KRON
 */

#ifdef	__cplusplus
extern "C" {
#endif

#define KRON_RX_MIN         4 /* минимальная длина кадра */
#define KRON_RX_MAX         250 /* максимальная длина кадра */
#define CHAR_TS             0x5A /* символ начала кадра */
#define CHAR_RS             0xA5 /* символ начала кадра */
#define KRON_BUFF_LEN       KRON_RX_MAX
#define RX_LENGTH           KRON_BUFF_LEN
#define TX_LENGTH           50
#define SYS_ADR             0xFF
#define BROAD_ADR           0xFE
#define MULTI_ADR           0x80

/* packet type */
#define DATA_PKT            1
#define USER_CFG            2
#define SYS_CFG             3
#define VER_PKT             4
#define ALG_PKT             5
#define EEPR_PKT            6

/* command type */
#define FIND_CMD            1
#define DEL_CMD             0x20 // ToDo: or 2?
#define SAVE_CMD            3
#define RST_CMD             4
//------------------------------------биты контроля
typedef union
{
	struct	{
		unsigned char FAN1_ON :1;
		unsigned char FAN2_ON :1;
		unsigned char FAN3_ROFF :1;
		unsigned char FAN4_RON :1;
		unsigned char RESERV1 :1;
		unsigned char RESERV2 :1;
		unsigned char RESERV3 :1;
		unsigned char RESERV4 :1;
	}bit;
	unsigned char byte;
}control_type;

//------------------------------------биты команды
typedef union
{
	struct	{
		unsigned char EEPROM :1;
		unsigned char ADR_SET :1;
		unsigned char IN_DATA :1;
		unsigned char OUT_DATA :1;
		unsigned char TE_DATA :1;
		unsigned char FAN_CONTROL :1;
		unsigned char DIAG_WIDE :1;
		unsigned char I0_SENSE :1;
	}bit;
	unsigned char byte;
}rx_cmd_type;
//------------------------------------биты режима работы
typedef union
	{
	struct	{
		unsigned char LCD_ON :1;
		unsigned char LED_ON :1;
		unsigned char PCC_ON :1;
		unsigned char DEBUG_ON :1;
		unsigned char GroupM :1;
		unsigned char EXT_Id :1;
		unsigned char EXTt_pol :1;
		unsigned char RELAY_MODE :1;
	}bit;
	unsigned char byte;
	}rx_mode_type;

//------------------------------------заголовок пакета
typedef	struct
	{
	unsigned char start;
	unsigned char dest_adr;
	unsigned char src_adr;
	unsigned char length;
	unsigned char number;
	unsigned char type;
	}header_type;

//------------------------------------пакет данных
typedef	struct
	{
	unsigned char cmd;
	control_type control;
	unsigned int setI;
	unsigned int setU;
	}rx_data_type;
	
typedef	struct
{
	unsigned char operation;
	unsigned char error;
	unsigned int I;
	unsigned int U;
	unsigned int Ip;
	unsigned int t1;
	unsigned int t2;
	unsigned char In_st;
	unsigned char Out_st;
}tx_data_type;

//------------------------------------пакет конфигурирования пользователя
typedef	struct
	{
	rx_cmd_type cmd;
	unsigned char Adress;
	unsigned int K_I;
	unsigned int K_U;
	unsigned int K_Ip;
	unsigned int K_Id;
	unsigned int B_I;
	unsigned int B_U;
	unsigned int B_Ip;
	unsigned int B_Id;
	unsigned char D_I;
	unsigned char D_U;
	unsigned char D_Ip;
	unsigned char D_Id;
	}rx_usr_type;
	
typedef	struct
	{
	unsigned char cmd;
	unsigned char new_adr;
	unsigned int K_I;
	unsigned int K_U;
	unsigned int K_Ip;
	unsigned int K_Id;
	unsigned int B_I;
	unsigned int B_U;
	unsigned int B_Ip;
	unsigned int B_Id;
	unsigned char D_I;
	unsigned char D_U;
	unsigned char D_Ip;
	unsigned char D_Id;
	}tx_usr_type;


//------------------------------------пакет конфигурирования системы
typedef	struct
	{
	rx_cmd_type cmd;
	rx_mode_type mode;
	unsigned int maxU;
	unsigned int maxI;
	unsigned int maxId;
	unsigned int maxPd;
	unsigned char dm_cnt;
	unsigned char slave_cnt_u;
	unsigned char slave_cnt_i;
	unsigned int cfg;
	unsigned char autostart_try;
	unsigned int restart_timout;
	unsigned int autostart_u;
	}rx_sys_type;
	
typedef	struct
	{
	rx_cmd_type cmd;
	rx_mode_type mode;
	unsigned int maxU;
	unsigned int maxI;
	unsigned int maxId;
	unsigned int maxPd;
	unsigned char dm_cnt; //13
	unsigned char slave_cnt_u; //14
	unsigned char slave_cnt_i; //15
	unsigned int cfg;  //16,17
	unsigned char autostart_try; //18
	unsigned int restart_timout; //19, 20
	unsigned int autostart_u; //21, 22
	}tx_sys_type;
//------------------------------------пакет с версией блока
typedef	struct
{
	rx_cmd_type cmd;
	unsigned char reserv;
	unsigned char number[8];
}rx_ver_type;

typedef	struct
{
	unsigned char hard_ver;
	unsigned char hard_mode;
	unsigned char soft_ver;
	unsigned char soft_mode;
	char number[8];
}tx_ver_type;
//---------------------------------пакет для загрузки алгоритмов
typedef	struct
	{
	unsigned char cmd;	
	unsigned char size;
	unsigned char data[32];
	}rx_alg_type; 
	
typedef	struct
	{
	unsigned char result;
	unsigned int mem_point;
	}tx_alg_type;

//---------------------------------пакет для передачи состояния EEPROM
typedef	struct
	{
	unsigned int ADR;
	unsigned char D[32];
	}tx_EEPROM_type;

//------------------------------------ПАКЕТ
typedef union
	{struct		 
		{
		header_type header;
		union
			{
			rx_sys_type rx_sys;
			rx_usr_type rx_usr;
			rx_data_type rx_data;
			rx_ver_type rx_ver;
			rx_alg_type rx_alg;
			}data;	
		}fld;
	unsigned char byte[RX_LENGTH];
	}rx_pack_type;
	
typedef union
	{struct
		{
		header_type header;
		union
			{
			tx_sys_type tx_sys;
			tx_usr_type tx_usr;
			tx_data_type tx_data;
			tx_ver_type tx_ver;
			tx_alg_type tx_alg;
			tx_EEPROM_type tx_EEPROM;
			}data;
		}fld;
	unsigned char byte[TX_LENGTH];
	}tx_pack_type;

#ifdef __cplusplus
}
#endif

#endif /* KRON_IMP_H */
