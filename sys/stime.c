#pragma message	("@(#)stime.c")

/*
 * Системное время
 */

#include <sys/system.h>

__no_init uint32_t stime; /* системное время, 100 HZ */

/*
 * Прерывание по событию:
 * переполнение системного таймера 10 ms.
 */
#pragma vector=SYSTRM(TIMER,_COMP_vect)
#pragma type_attribute=__interrupt
void sys_tick_irq(void)
{
    stime++;
}

bool run_time(uint32_t start, uint32_t delta)
{
    uint32_t curtime;
    uint32_t runtime = start + delta;
    get_stime(curtime);
    if (runtime < start) { /* run after overflow */
        if ((curtime >= runtime) && (curtime < start)) {
            return true;
        }
    } else { /* run before overflow */
        if ((curtime >= runtime) || (curtime < start)) {
            return true;
        }
    }
    return false;
}

stime_t get_fin_time(uint32_t delay)
{
	stime_t time;
	get_stime(time.run);
	time.del = delay;
	return time;
}

uint32_t get_time_left(stime_t time)
{
    uint32_t del, t;
 	get_stime(t);
    if (t < time.run) { // overflow system timer
 		del = (UINT32_MAX - time.run) + t;
 	} else {
 		del = t - time.run;
 	}
 	if (del > time.del) return 0;
 	else return (time.del - del);
}

uint32_t get_interval(uint32_t run) {
	uint32_t del, t;
 	get_stime(t);
 	if (t < run) // overflow system timer
 		del = (UINT32_MAX - run) + t;
 	else del = t - run;
 	return del;
}