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

#define fatal(...) \
  do { \
    fprintf(stderr, __VA_ARGS__); \
    exit(EXIT_FAILURE); \
  } while (0)

#ifdef MTIME_TOOL
static inline void die(const char msg[]) {
  perror(msg);
  exit(EXIT_FAILURE);
}
#else
static inline void die(const char msg[]) {
  perror(msg);
  abort();
}
#endif

static inline long get_time_monotonic_ns(void) {
  struct timespec t;
  if (clock_gettime(CLOCK_MONOTONIC, &t))
    die("clock_gettime failed");
  return t.tv_nsec + t.tv_sec * 1e9;
}

int mtime_init(char *envp[]) {
  environ = envp;
  return 0;
}

int profile_cmd(char **cmd, long *runtime, struct rusage *r_usage) {
  struct sigaction oact;
  if (sigaction(SIGINT, &(struct sigaction){ .sa_handler = SIG_IGN }, &oact))
    return -1;
  pid_t child_pid = fork();
  *runtime = get_time_monotonic_ns();
  if (child_pid == 0) {
    if (sigaction(SIGINT, &(struct sigaction){ .sa_handler = SIG_DFL }, NULL))
      exit(EXIT_FAILURE);
    execvp(*cmd, cmd);
    exit(EXIT_FAILURE);
  }
  if (child_pid == -1)  {
    if (sigaction(SIGINT, &oact, NULL))
      die("sigaction failed");
    return -1;
  }
  
  int status;
  if (waitpid(child_pid, &status, 0) == -1)
    return -1;
  *runtime = get_time_monotonic_ns() - *runtime;
  if (sigaction(SIGINT, &oact, NULL))
    die("sigaction failed");
  status = WEXITSTATUS(status);

  if (getrusage(RUSAGE_CHILDREN, r_usage))
    return status;

  return status; 
}

#ifdef MTIME_TOOL
int main(int argc, char *argv[], char *envp[]) {
  if (argc < 2)
    fatal("Usage: %s command [args ...]\n", *argv);

  if (mtime_init(envp))
    die("mtime_init failed");

  struct rusage r_usage;
  long runtime;
  if (profile_cmd(argv + 1, &runtime, &r_usage) == -1)
    die("profile_cmd failed");
  
  double user_cpu_time = r_usage.ru_utime.tv_sec + r_usage.ru_utime.tv_usec * 1e-6;
  double system_cpu_time =
      r_usage.ru_stime.tv_sec + r_usage.ru_stime.tv_usec * 1e-6;
  double total_time = runtime * 1e-9;

  if (putchar('\n') == EOF)
    die("putchar failed");


  while (*(++argv)) {
    if (printf("%s ", *argv) < 0)
      die("printf failed");
  }

  if (printf(" %.9fs user %.9fs system %.9f%% cpu %.9fs total\n",
             user_cpu_time, system_cpu_time,
             (100.0 * (user_cpu_time + system_cpu_time) /
                   total_time),
             total_time) < 0)
    die("printf failed");

  if (fflush(stdout))
    die("fflush failed");

  return EXIT_SUCCESS;
}
#endif
