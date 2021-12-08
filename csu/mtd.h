#ifndef MTD_H_
#define MTD_H_
#pragma message	("@(#)mtd.h")

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
	unsigned char T		:1;
	unsigned char I		:1;	
	unsigned char U		:1;
	unsigned char dU	:1;
	unsigned char t		:1;
	unsigned char C		:1;
	unsigned char reserv1 :1;
	unsigned char reserv2 :1;
} stop_flag_t; 

typedef union {
	struct {
		unsigned char data_type;//1
		unsigned char type; //2
		stop_flag_t stop_flag; //3
		unsigned int I_ch; //4,5
		unsigned int U_ch; //6,7
		unsigned int I_dch; //8,9
		unsigned int U_dch; //10,11
		unsigned char T_ch; //12
		unsigned char T_dch; //13
		unsigned int end_I; //14, 15
		unsigned int end_U; //16, 17
		unsigned int end_dU; //18, 19
		unsigned int end_Temp; //20,22
		unsigned int end_C;   //22, 23
		unsigned char end_H; //24
		unsigned char end_M; //25
		unsigned char end_S; //26
	} fld;
	unsigned char byte[32];
	unsigned int word[16];
} stg_t;

typedef union {
	struct {
		unsigned char data_type;//1
		unsigned char name[17]; //2-18
		unsigned int Im; //19, 20
		unsigned int Um; //21, 22
		unsigned char Hm;//23
		unsigned char Mm;//24
		unsigned char Sm;//25
		unsigned char Cnt;//26
		unsigned char Nstage;//27
		//unsigned int Next_Method; //28, 29
	} fld;
	unsigned char byte[32];
	unsigned int word[16];
} mtd_t;
	

typedef struct {
	unsigned int I;
	unsigned int U;
	unsigned int dU;
	unsigned int max_U;
} finish_t;

#define MTD_N       15


unsigned char finish_conditions(void);
void stage_status(void);
void calculate_method(void);
void calculate_stage(void);
void read_method(void);
void read_stage(unsigned char stage);
void Start_method(unsigned char new);
void Stop_method(void);
void create_method(unsigned char method);
unsigned int find_free_memory(unsigned char m_cnt);
void delete_all_method(void);

//extern unsigned char method_cnt, stage_cnt, cycle_cnt;
extern uint8_t mCnt, sCnt, cCnt;
extern unsigned int Method_ARD[MTD_N];
extern unsigned int Wr_ADR;
extern stg_t Stage;
extern mtd_t Method;
extern finish_t finish;

#ifdef __cplusplus
}
#endif

#endif /* MTD_H */