#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "mtime.h"

extern char **environ;
static pid_t parent_pid;

#define fatal(...)                                                             \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

static inline void die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static inline long get_time_monotonic_ns(void) {
  struct timespec t;
  if (clock_gettime(CLOCK_MONOTONIC, &t))
    die("clock_gettime failed");
  return t.tv_nsec + t.tv_sec * 1e9;
}

static void sigint_handler(int signum) {
  if (getpid() == parent_pid) {
    if (putc('\n', stdout) == EOF)
      die("putc failed");
    return;
  }
  if (kill(getpid(), SIGINT))
    die("kill failed");
}

int mtime_init(char *envp[]) {
  environ = envp;
  parent_pid = getpid();

  struct sigaction sa = { 
    .sa_handler = sigint_handler,
    .sa_flags = SA_RESTART };

  if (sigaction(SIGINT, &sa, NULL))
    return -1;

  return 0;
}

int profile_cmd(char **cmd, long *runtime, struct rusage *r_usage) {
  pid_t child_pid = fork();
  *runtime = get_time_monotonic_ns();
  if (child_pid == 0) {
    if (execvp(*cmd, cmd))
      return -1;
    exit(EXIT_SUCCESS);
  }
  if (child_pid == -1)
    return -1;
  
  int status;
  if (waitpid(child_pid, &status, 0) == -1)
    return -1;
  *runtime = get_time_monotonic_ns() - *runtime;
  if (WEXITSTATUS(status))
    exit(EXIT_FAILURE);

  if (getrusage(RUSAGE_CHILDREN, r_usage))
    return -1;

  return 0; 
}

#ifdef MTIME_TOOL
int main(int argc, char *argv[], char *envp[]) {
  if (argc < 2)
    fatal("Usage: %s command [args ...]\n", *argv);

  if (mtime_init(envp))
    die("mtime_init failed");

  struct rusage r_usage;
  long runtime;
  if (profile_cmd(argv + 1, &runtime, &r_usage))
    die("profile_cmd failed");
  
  double user_cpu_time = r_usage.ru_utime.tv_sec + r_usage.ru_utime.tv_usec * 1e-6;
  double system_cpu_time =
      r_usage.ru_stime.tv_sec + r_usage.ru_stime.tv_usec * 1e-6;
  double total_time = runtime * 1e-9;
 
  while (*(++argv)) {
    if (printf("%s ", *argv) < 0)
      die("printf failed");
  }

  if (printf(" %.9fs user %.9fs system %d%% cpu %.9fs total\n",
             user_cpu_time, system_cpu_time,
             (int)(100 * (user_cpu_time + system_cpu_time) /
                   total_time),
             total_time) < 0)
    die("printf failed");

  if (fflush(stdout))
    die("fflush failed");

  return EXIT_SUCCESS;
}
#endif
