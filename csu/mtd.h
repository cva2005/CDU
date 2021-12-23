#ifndef MTD_H_
#define MTD_H_
#pragma message	("@(#)mtd.h")

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t h;
    uint8_t m;
    uint8_t s;
} hms_t;

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
		csu_st type; //2
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
        hms_t end;
		//unsigned char end_H; //24
		//unsigned char end_M; //25
		//unsigned char end_S; //26
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
        hms_t end;
		//unsigned char Hm;//23
		//unsigned char Mm;//24
		//unsigned char Sm;//25
		unsigned char Cnt;//26
		unsigned char NStg;//27
		//unsigned int Next_Mtd; //28, 29
	} fld;
	unsigned char byte[32];
	unsigned int word[16];
} mtd_t;
	

typedef struct {
	unsigned int I;
	unsigned int U;
	unsigned int dU;
	unsigned int max_U;
} fin_t;

#define MTD_N           15
#define DEF_MTD_Stg     1
#define INF_TIME        100
#define DU_TIME         SEC(400UL)

bool fin_cond (void);
void stg_status (void);
void calc_mtd (void);
void calc_stg (void);
void read_mtd (void);
void read_stg (unsigned char num);
void start_mtd (unsigned char num);
void stop_mtd (void);
void create_mtd (unsigned char num);
uint8_t find_free (void);
void delete_all_mtd (void);

extern unsigned int set_I, set_Id, set_U, set_UmemC, set_UmemD;
extern unsigned int max_set_I, max_set_Id, max_set_U;
extern uint8_t mCnt, sCnt, cCnt;
extern uint8_t StgNum[MTD_N];
extern unsigned int msNum;
extern stg_t Stg;
extern mtd_t Mtd;
extern fin_t Fin;
extern bool SaveMtd;
extern uint8_t Hour, Min, Sec, Hour_Stg, Min_Stg, Sec_Stg;

#ifdef __cplusplus
}
#endif

#endif /* MTD_H */