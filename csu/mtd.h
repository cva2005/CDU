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
    uint8_t T       :1;
    uint8_t I       :1;	
    uint8_t U       :1;
    uint8_t dU      :1;
    uint8_t t       :1;
    uint8_t C       :1;
    uint8_t rsrv    :2;
} stop_flag_t; 

typedef union {
    struct {
        uint8_t data_type;//1
        csu_st type; //2
        stop_flag_t stop_flag; //3
        uint16_t I_ch; //4,5
        uint16_t U_ch; //6,7
        uint16_t I_dch; //8,9
        uint16_t U_dch; //10,11
        uint8_t T_ch; //12
        uint8_t T_dch; //13
        uint16_t end_I; //14, 15
        uint16_t end_U; //16, 17
        uint16_t end_dU; //18, 19
        uint16_t end_Temp; //20,22
        uint16_t end_C;   //22, 23
        hms_t end;
    } fld;
    uint8_t byte[32];
    uint16_t word[16];
} stg_t;

typedef union {
    struct {
        uint8_t data_type;//1
        uint8_t name[17]; //2-18
        uint16_t Im; //19, 20
        uint16_t Um; //21, 22
        hms_t end;
        uint8_t Cnt;//26
        uint8_t NStg;//27
	} fld;
    uint8_t byte[32];
    uint16_t word[16];
} mtd_t;
	

typedef struct {
    uint16_t I;
    uint16_t U;
    uint16_t dU;
    uint16_t max_U;
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
void read_stg (uint8_t num);
void start_mtd (uint8_t num);
void stop_mtd (void);
void create_mtd (uint8_t num);
uint8_t find_free (void);
void delete_all_mtd (void);

extern uint8_t mCnt, sCnt, cCnt;
extern uint8_t StgNum[MTD_N];
extern uint16_t msNum;
extern stg_t Stg;
extern mtd_t Mtd;
extern fin_t Fin;
extern bool SaveMtd;
extern hms_t Tm, Ts;

#ifdef __cplusplus
}
#endif

#endif /* MTD_H */