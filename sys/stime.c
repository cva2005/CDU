#pragma message	("@(#)stime.c     1.00    11/09/12 OWEN")

/*
 * ????????? ?????
 */

#include <sys/system.h>

__no_init unsigned short stime; /* ????????? ?????, 100 HZ */

/*
 * ?????????? ?? ???????:
 * ???????????? ?????????? ??????? 10 ms.
 */
#pragma vector=SYSTRM(TIMER,_COMP_vect)
#pragma type_attribute=__interrupt
void sys_tick_irq(void)
{
    stime++;
}

bool run_time(unsigned short start, unsigned short delta)
{
    unsigned short curtime;
    unsigned short runtime = start + delta;

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