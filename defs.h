#include <stdio.h>
#include <stdint.h>
#include "vcd.h"

typedef uint64_t cm_t;
typedef uint32_t word_t;

#define CM_SUB    0400000000000ULL
#define CM_EAE    0200000000000ULL
#define CM_EAE_R  0100000000000ULL
#define CM_MBO    0040000000000ULL
#define CM_MBI    0020000000000ULL
#define CM_SKPI   0010000000000ULL
#define CM_ACO    0004000000000ULL
#define CM_ACI    0002000000000ULL
#define CM_IRI    0001000000000ULL
#define CM_ARO    0000400000000ULL
#define CM_ARI    0000200000000ULL
#define CM_CJIT   0000100000000ULL
#define CM_PCO    0000040000000ULL
#define CM_PCI    0000020000000ULL
#define CM_EAE_P  0000010000000ULL
#define CM_MQO    0000004000000ULL
#define CM_MQI    0000002000000ULL
#define CM_DEI    0000001000000ULL
#define CM_LI     0000000400000ULL
#define CM_AND    0000000200000ULL
#define CM_TI     0000000100000ULL
#define CM_DONE   0000000040000ULL
#define CM_AXS    0000000020000ULL
#define CM_CMA0   0000000010000ULL
#define CM_CMA1   0000000004000ULL
#define CM_CMA2   0000000002000ULL
#define CM_CMA3   0000000001000ULL
#define CM_CMA4   0000000000400ULL
#define CM_CMA5   0000000000200ULL
#define CM_CMA    (CM_CMA0|CM_CMA1|CM_CMA2|CM_CMA3|CM_CMA4|CM_CMA5)
#define CM_EXT    0000000000100ULL
#define CM_CONT   0000000000040ULL
#define CM_PLUS1  0000000000020ULL
#define CM_SM     0000000000010ULL
#define CM_ADSO   0000000000004ULL
#define CM_KEY    0000000000002ULL
#define CM_DCH    0000000000001ULL

extern long long nanoseconds;

extern cm_t cm[];
extern unsigned sig_KRI, sig_KIO, sig_KCT, sig_KSP, sig_KST;
extern unsigned sig_KDP, sig_KEX, sig_KDN, sig_KEN;
extern unsigned sig_SW_SGL_INST, sig_SW_SGL_STP;
extern word_t MB, AC, AR, PC, MQ, IR, MA, ADDR_SW, DATA_SW;
extern unsigned ff_RUN, ff_SAO;
extern cm_t sig_CMSL, ff_CMA, ff_SM, ff_CONT, ff_DONE;
extern cm_t ff_IRI, ff_MBI, ff_ACI, ff_ARI, PCI;
extern word_t MA, SA;

#ifdef DEBUG_VCD
#define VCD(IDX, VAL) vcd_value(nanoseconds, vcd_##IDX, VAL)
#define VAR(FOO) VCD(FOO, FOO)
#define FF(FOO)  VCD(FOO, ff_##FOO)
#define SIG(FOO) VCD(FOO, sig_##FOO)
#else
#define VCD(IDX, VAL) do { VAL; } while (0)
#define VAR(FOO) do {} while (0)
#define FF(FOO)  do {} while (0)
#define SIG(FOO) do {} while (0)
#endif

#ifdef DEBUG_VCD
extern int vcd_CLK, vcd_CM_STROBE, vcd_CLR;
extern int vcd_MEM_STROBE, vcd_MEM_WRITE, vcd_SA, vcd_MA;
extern int vcd_IR, vcd_MB, vcd_AC, vcd_AR, vcd_PC, vcd_RUN;
extern int vcd_CMA, vcd_SM, vcd_CONT, vcd_REP, vcd_SAO;
extern int vcd_IRI, vcd_MBI, vcd_ACI, vcd_ARI, vcd_PCI;
extern int vcd_MBO, vcd_ACO, vcd_ARO, vcd_PCO;
#endif

extern void panel(void);
extern void quit(int);
extern void power_on(void);
extern void power_off(void);
extern void cp_clk(void);
extern void cm_clk_pos(void);
extern void clr(void);
extern void sync_clk(void);

extern void ptr_mount(FILE *f);
