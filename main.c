#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include "defs.h"

long long nanoseconds;
static struct timeval previous;

/*
Nanoseconds     Event                   Fetch   Defer   Execute/IA0
  0000          CLK
  0100          MA JAM
                CM STROBE		21/CONT	31	30	32/CONT
  0185		DIGIT READ -> 550
  0250          MEM DONE -> 970
  0280		WORD READ -> 550
  0312          CM STROBE               12	-	-	23/CONT
  0488          CLR
  0500		IO RESTART
  0510          MEM STROBE -> 600
  0700          CM STROBE		24/SM	24/SM	   60-77/CONT
  0765          MEM WRITE <- 25
  0900          CM STROBE		-	-	   10/SM
  1000          Next cycle.

  -------------------------

  0000          CLK		Break		Key
  0100          CM STROBE	  11/SM		01/SM  03/SM
  0300          CM STROBE	  -
  0400          CLR
  0700          CM STROBE	  -
  0900          CM STROBE	  -
  1100          CM STROBE	  34     37	   25
  1300          CM STROBE	  -
  1400          CLR
  1700          CM STROBE	14/SM  13/CONT
  1900          CM STROBE	-      16/SM
  2100          CM STROBE	  37   36
  2300          CM STROBE	
  2400          CLR
  2700          CM STROBE	       17
  2900          CM STROBE		
 */

#ifdef DEBUG_VCD
int vcd_CLK, vcd_CM_STROBE, vcd_CLR;
int vcd_MEM_STROBE, vcd_MEM_WRITE, vcd_SA, vcd_MA;
int vcd_IR, vcd_MB, vcd_AC, vcd_AR, vcd_PC, vcd_RUN;
int vcd_CMA, vcd_SM, vcd_CONT, vcd_REP, vcd_SAO;
int vcd_IRI, vcd_MBI, vcd_ACI, vcd_ARI, vcd_PCI;
int vcd_MBO, vcd_ACO, vcd_ARO, vcd_PCO;
#endif

static void delay(int microseconds)
{
  struct timeval tv;
  useconds_t x;
  gettimeofday(&tv, NULL);
  x = 1000000 * (tv.tv_sec - previous.tv_sec) + tv.tv_usec - previous.tv_usec;
  printf("%d %d\n", (int)microseconds, (int)x);
  if (x < microseconds)
    usleep(microseconds - x);
  gettimeofday(&previous, NULL);
}

static void timing_chain(void)
{
  unsigned memory_access;

  cp_clk();

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
  vcd_CLK = vcd_variable("CLK", "reg", 1);
  vcd_CM_STROBE = vcd_variable("CM_STROBE", "reg", 1);
  vcd_CLR = vcd_variable("CLR", "reg", 1);
  vcd_MEM_STROBE = vcd_variable("MEM_STROBE", "reg", 1);
  vcd_MEM_WRITE = vcd_variable("MEM_WRITE", "reg", 1);
  vcd_SA = vcd_variable("SA", "reg", 18);
  vcd_MA = vcd_variable("MA", "reg", 13);
  vcd_IR = vcd_variable("IR", "reg", 5);
  vcd_MB = vcd_variable("MB", "reg", 18);
  vcd_AC = vcd_variable("AC", "reg", 18);
  vcd_AR = vcd_variable("AR", "reg", 18);
  vcd_PC = vcd_variable("PC", "reg", 13);
  vcd_RUN = vcd_variable("RUN", "reg", 1);
  vcd_SM = vcd_variable("SM", "reg", 1);
  vcd_CONT = vcd_variable("CONT", "reg", 1);
  vcd_CMA = vcd_variable("CMA", "reg", 6);
  vcd_IRI = vcd_variable("IRI", "reg", 1);
  vcd_MBI = vcd_variable("MBI", "reg", 1);
  vcd_MBO = vcd_variable("MBO", "reg", 1);
  vcd_ACI = vcd_variable("ACI", "reg", 1);
  vcd_ACO = vcd_variable("ACO", "reg", 1);
  vcd_ARI = vcd_variable("ARI", "reg", 1);
  vcd_ARO = vcd_variable("ARO", "reg", 1);
  vcd_PCI = vcd_variable("PCI", "reg", 1);
  vcd_PCO = vcd_variable("PCO", "reg", 1);
  vcd_SAO = vcd_variable("SAO", "reg", 1);
  vcd_REP = vcd_variable("REP", "reg", 1);
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
