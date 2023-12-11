#ifndef MTIME_H
#define MTIME_H

#include <sys/resource.h>

int mtime_init(char *envp[]);

int profile_cmd(char *cmd[], long *runtime, struct rusage *r_usage);

#endif
