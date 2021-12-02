#ifndef PID_R_H
#define PID_R_H

#ifdef	__cplusplus
extern "C"
{
#endif

#define ST_SIZE 4

typedef struct {
	float Kp;			/* gain factor */
	float Ti;			/* integration time */
	float Tf;			/* derivative filter tau */
	float Td;			/* derivative time */
	float i[ST_SIZE];	/* old input states */
	float u;			/* old output state */
	float d;			/* old derivative state */
	float Xd;			/* dead zone */
	float Xi;			/* integral zone */
} pid_r_instance;


void pid_r_init (pid_r_instance *S);
float pid_r (pid_r_instance *S, float in);

#ifdef	__cplusplus
}
#endif

#endif /* PID_R_H */
