#include <stdio.h>
#include <stdint.h>
#include "vcd.h"

typedef uint64_t cm_t;
typedef uint32_t word_t;

extern long long nanoseconds;

extern cm_t cm[];
extern volatile unsigned sig_KRI, sig_KIO, sig_KCT, sig_KSP, sig_KST;
extern volatile unsigned sig_KDP, sig_KEX, sig_KDN, sig_KEN;
extern volatile unsigned sig_SW_SGL_INST, sig_SW_SGL_STP;
extern unsigned sig_KIOA3, sig_KIOA4, sig_KIOA5;
extern word_t MB, AC, AR, PC, MQ, IR, ADDR_SW, DATA_SW;
extern unsigned ff_RUN, KIOA5;
extern cm_t sig_CMSL, ff_CMA, ff_SM, ff_CONT, ff_MBI;
extern word_t MA, SA;

#ifdef DEBUG_VCD
#define VCD(IDX, VAL) vcd_value(nanoseconds, vcd_##IDX, VAL)
#define VAR(FOO) VCD(FOO, FOO)
#define FF(FOO)  VCD(FOO, ff_##FOO)
#define SIG(FOO) VCD(FOO, sig_##FOO)

#define VCD_VARIABLES \
  X(CLK, 1) \
  X(CM_STROBE, 1) \
  X(CLR, 1) \
  X(MEM_STROBE, 1) \
  X(MEM_WRITE, 1) \
  X(SA, 18) \
  X(MA, 13) \
  X(IR, 5) \
  X(MB, 18) \
  X(AC, 18) \
  X(AR, 18) \
  X(PC, 13) \
  X(RUN, 1) \
  X(SM, 1) \
  X(CONT, 1) \
  X(KEY, 1) \
  X(AXS, 1) \
  X(CMA, 6) \
  X(IRI, 1) \
  X(MBI, 1) \
  X(MBO, 1) \
  X(ACI, 1) \
  X(ACO, 1) \
  X(ARI, 1) \
  X(ARO, 1) \
  X(PCI, 1) \
  X(PCO, 1) \
  X(CJIT, 1) \
  X(PLUS1, 1) \
  X(SKPI, 1) \
  X(SAO, 1) \
  X(REP, 1) \
  X(ISZ, 1) \
  X(SKIP, 1) \
  X(CI17, 1) \
  X(KST, 1) \
  X(KSP, 1) \
  X(KCT, 1) \
  X(KDP, 1) \
  X(KDN, 1) \
  X(KEX, 1) \
  X(KEN, 1) \
  X(KRI, 1) \
  X(KIOA3, 1) \
  X(KIOA4, 1) \
  X(KIOA5, 1)
#else
#define VCD(IDX, VAL) do { VAL; } while (0)
#define VAR(FOO) do {} while (0)
#define FF(FOO)  do {} while (0)
#define SIG(FOO) do {} while (0)
#define VCD_VARIABLES
#endif

#define X(NAME, BITS) extern int vcd_##NAME;
VCD_VARIABLES
#undef X

extern void panel(void);
extern void quit(int);
extern void power_on(void);
extern void power_off(void);
extern unsigned cp_clk(void);
extern void cm_clk_pos(void);
extern void clr(void);
extern void sync_clk(void);
extern void mem_write(void);

extern void ptr_mount(FILE *f);
