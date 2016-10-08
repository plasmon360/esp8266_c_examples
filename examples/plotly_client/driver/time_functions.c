#include "os_type.h"
#include "osapi.h"
#include "time.h" // I use this only to make time structure from time since epoch or vice versa (mktime and gmtime), I cannot seem to use the other functions such as strftime as  I am getting an error "undefined reference to `_sbrk_r'"

//sntp timezone settings
uint32 sntptime_since_epoch;
uint32 localtime_since_epoch;
sint8 utc_timezone =  0; // will use this for sntp_timezone function for returning UTC time instead of default UTC+8 (china)
sint8 timezone_offset = -8;// anywhere between -11 to 13 with respect to GMT/UTC. Pacific time is GMT-8
sint8 dst_offset;
sint8 dst_offset_hours = 1;  // How many hours you want to offset. In usa it is 1 hour


// time.h
time_t rawtime_since_epoch, rawtime_since_epoch_with_dst;
struct tm * current_time_without_dst;
struct tm * current_time_with_dst;
struct tm dst_start;
struct tm dst_end;


// Function that will needs to be run to before using any of the functions below
void ICACHE_FLASH_ATTR my_sntp_init()
{
    // Set the servers name for the sntp time
    sntp_setservername(0, "us.pool.ntp.org"); 
    sntp_setservername(1, "montpelier.caltech.edu"); 
    sntp_setservername(2,"time-d.nist.gov");

    sntp_stop();
    sntp_set_timezone(utc_timezone);// set the timezone to utc
    sntp_init();
    // Parameters of the dst start and end. We set all parameters that are constant. We set year and the day when dst starts and ends in 	dst_check method.
    dst_start.tm_mon=4-1; // April is 3, becase tm_mon is 0 for january
    dst_start.tm_hour=2;//at 2 AM
    dst_start.tm_min=0;
    dst_start.tm_sec=0;
    dst_start.tm_isdst = -1;
    // Parameters of the dst end.
    dst_end.tm_mon=11-1; // Nov is 10
    dst_end.tm_hour=2;//at 2 AM
    dst_end.tm_min=0;
    dst_start.tm_sec=0;
    dst_end.tm_isdst = -1;
}

// http://hackaday.com/2012/07/16/automatic-daylight-savings-time-compensation-for-your-clock-projects/
// The following code will help in determining second sunday in april and first sunday in nov.
int ICACHE_FLASH_ATTR dow(int y, int m, int d)
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}
int ICACHE_FLASH_ATTR NthDate(int year, int month, int DOW, int NthWeek)
{
    int targetDate = 1;
    int firstDOW = dow(year,month,targetDate);
    while (firstDOW != DOW)
    {
        firstDOW = (firstDOW+1)%7;
        targetDate++;
    }
    //Adjust for weeks
    targetDate += (NthWeek-1)*7;
    return targetDate;
}
//


sint8 ICACHE_FLASH_ATTR dst_check(struct tm * timeptr)
// Check if the time represetned by timeptr is within daylight savings or not and returns dst_offset
{
    dst_start.tm_year = timeptr->tm_year;
    dst_end.tm_year = timeptr->tm_year;
    sint16 year =  (sint16)1900+timeptr->tm_year;
    dst_start.tm_mday= NthDate(year, dst_end.tm_mon+1, 0, 2); // returns the day of the second(2) sunday(0) of April(4) for a given yea
    dst_end.tm_mday=NthDate(year, dst_end.tm_mon+1, 0, 1); // returns the day of the first(1) sunday(0) of November(11) for a given year
    os_printf ("DST is observed between: %4d-%02d-%02d  %02d:%02d:%02d %4d-%02d-%02d  %02d:%02d:%02d    \n",
               1900+dst_start.tm_year, dst_start.tm_mon+1,dst_start.tm_mday,dst_start.tm_hour,
               dst_start.tm_min,dst_start.tm_sec, 1900+dst_end.tm_year, dst_end.tm_mon+1,
               dst_end.tm_mday,dst_end.tm_hour, dst_end.tm_min,dst_end.tm_sec);
    if (mktime(timeptr)>=mktime(&dst_start) && mktime(timeptr)<=mktime(&dst_end))
        dst_offset = dst_offset_hours; // set the dst offset
    else
        dst_offset = 0; // no dst offset
    return dst_offset;
}


struct tm * ICACHE_FLASH_ATTR sntp_current_time(void *arg)
// Callback Function that will return the timestamp
{
    sntptime_since_epoch =  sntp_get_current_timestamp(); // Get the UTC time since epoch from sntp servers
    localtime_since_epoch = sntptime_since_epoch+(timezone_offset)*3600;// compensate for time zone
    rawtime_since_epoch = (time_t) localtime_since_epoch; // type cast to time_t type
    current_time_without_dst = gmtime(&rawtime_since_epoch);
    dst_offset = dst_check(current_time_without_dst);
    if (dst_offset==1)
    {
        os_printf("You are in dst period. %d hours are added\n", dst_offset_hours);
    }
    else if (dst_offset ==0)
    {
        os_printf("You are not in dst period\n");
    }
    rawtime_since_epoch_with_dst = rawtime_since_epoch+dst_offset*3600; // compensate for dst_offset_hours
    current_time_with_dst = gmtime(&rawtime_since_epoch_with_dst);
    // Print Local time in yyyy-mm-dd HH:MM:SS
    os_printf ("Local time : %4d-%02d-%02d  %02d:%02d:%02d\n",
               1900+current_time_with_dst->tm_year, current_time_with_dst->tm_mon+1,
               current_time_with_dst->tm_mday,current_time_with_dst->tm_hour,
               current_time_with_dst->tm_min,current_time_with_dst->tm_sec );
   return current_time_with_dst;
}

