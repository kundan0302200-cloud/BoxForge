#ifndef BOXFORGE_LIFECYCLE_H
#define BOXFORGE_LIFECYCLE_H
#include <sys/types.h>
int lifecycle_init(void);
int lifecycle_wait(pid_t pid, int *status);
#endif
