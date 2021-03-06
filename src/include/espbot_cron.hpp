/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_CRON_HPP__
#define __APP_CRON_HPP__

#define CRON_MAX_JOBS 30

struct date
{
    int year;
    char month;
    char day_of_month;
    char hours;
    char minutes;
    char seconds;
    char day_of_week;
    uint32 timestamp;
};

void cron_init(void); 

/*
 * will sync to time provided by NTP
 * once synced job execution will start
 */
void cron_sync(void); 

#define CRON_STAR 0xFF

/*
 * # job definition:
 * # .---------------- minute (0 - 59)
 * # |  .------------- hour (0 - 23)
 * # |  |  .---------- day of month (1 - 31)
 * # |  |  |  .------- month (1 - 12) OR jan,feb,mar,apr ...
 * # |  |  |  |  .---- day of week (1 - 7) mon,tue,wed,thu,fri,sat,sun
 * # |  |  |  |  |
 * # *  *  *  *  * funcntion
 * 
 * result: > 0  -> job id
 *         < 0  -> error
 */

int cron_add_job(char min, char hour, char day_of_month, char month, char day_of_week, void (*command)(void *), void *param);

/*
 * delete job_id
 */
void cron_del_job(int job_id);

/*
 * get current time as struct date
 */
struct date *cron_get_current_time(void);

/*
 * force initialization of current time before than cron execution
 */
void cron_init_current_time(void);


/*
 * CONFIGURATION & PERSISTENCY
 */
void cron_enable(void);
void cron_disable(void);
void cron_start(void);
void cron_stop(void);
bool cron_enabled(void);

char *cron_cfg_json_stringify(char *dest = NULL, int len = 0);
int cron_cfg_save(void);

/*
 * DEBUG
 */
void cron_print_jobs(void);

#endif