
struct tm * timeptr;
void my_sntp_init();
sint8 dst_check(struct tm * timeptr);
struct tm * sntp_current_time(void *arg);
