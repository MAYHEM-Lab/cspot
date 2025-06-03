#ifndef CMQ_CACHE_H
#define CMQ_CACHE_H

int cmq_sd_cache_find(char *ip_str, unsigned short port);
int cmq_sd_cache_insert(char *ip_str, unsigned short port, int sd);
int cmq_sd_cache_idle(int sd);
void cmq_sd_cache_destroy(int sd);

#endif

