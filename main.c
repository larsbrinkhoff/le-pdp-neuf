#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include "defs.h"

long long nanoseconds;
static struct timeval previous;

#define X(NAME, BITS) int vcd_##NAME;
VCD_VARIABLES
#undef X

static void delay(int microseconds)
{
  struct timeval tv;
  useconds_t x;
  gettimeofday(&tv, NULL);
  x = 1000000 * (tv.tv_sec - previous.tv_sec);
  x += tv.tv_usec - previous.tv_usec;
  if (x < microseconds)
    usleep(microseconds - x);
  gettimeofday(&previous, NULL);
}

static void timing_chain(void)
{
  unsigned memory_access, sig_key_init_pos;

  sig_key_init_pos = cp_clk();

  VCD(CLK, ff_RUN);
  nanoseconds += 10; //10
  VCD(CLK, 0);

  memory_access = ff_SM;
  if (memory_access)
    sync_clk();

  nanoseconds += 15; //25
  VCD(MEM_WRITE, 0);

  nanoseconds += 75; //100
  if (ff_RUN)
    cm_clk_pos();
  nanoseconds += 10; //110
  VCD(CM_STROBE, 0);

  nanoseconds += 202; //312
  if (ff_RUN && ff_CONT)
    cm_clk_pos();
  nanoseconds += 10; //322
  VCD(CM_STROBE, 0);

  nanoseconds += 166; //488
  if (memory_access)
    clr();

  nanoseconds += 22; //510
  VCD(MEM_STROBE, memory_access);
  nanoseconds += 78; //588
  VCD(CLR, 0);
  VCD(MBI, ff_MBI = 0);
  nanoseconds += 12; //600
  VCD(MEM_STROBE, 0);

  nanoseconds += 100; //700
  if (ff_RUN && (ff_CONT || memory_access))
    cm_clk_pos();
  nanoseconds += 10; //710
  VCD(CM_STROBE, 0);

  nanoseconds += 55; //765
  if (memory_access) {
    VCD(MEM_WRITE, 1);
    mem_write();
  }

  nanoseconds += 135; //900
  if (ff_RUN && ff_CONT)
    cm_clk_pos();
  nanoseconds += 10; //910
  VCD(CM_STROBE, 0);
  nanoseconds += 90;
}

int main(int argc, char **argv)
{
  int i;

#ifdef DEBUG_VCD
#define X(NAME, BITS) vcd_##NAME = vcd_variable(#NAME, "reg", BITS);
VCD_VARIABLES
#undef X
  vcd_start("dump.vcd", "PDP-9 simulator", "1ns");
#endif

  panel();
  signal(SIGINT, quit);
  signal(SIGQUIT, quit);
  nanoseconds = 0;
  gettimeofday(&previous, NULL);
  for (i = 0;; i++) {
    if (i == 10000) {
      delay(i);
      i = 0;
    }
    timing_chain();
  }
  return 0;
}
