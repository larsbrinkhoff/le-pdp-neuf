/* Central processor. */

#include <stdio.h>
#include "defs.h"

#define IR0 (IR & 020)
#define IR1 (IR & 010)
#define IR2 (IR & 004)
#define IR3 (IR & 002)
#define IR4 (IR & 001)

#define MB07 (MB & 040)
#define MB12 (MB & 040)
#define MB13 (MB & 040)
#define MB14 (MB & 040)

word_t IR;
word_t MB;
word_t AR;
word_t AC = 0777;
word_t MQ;
word_t PC;
unsigned LINK;

word_t ADDR_SW;
word_t DATA_SW;

word_t sig_A_BUS;
word_t sig_B_BUS;
word_t sig_O_BUS;

word_t sig_ADR;
unsigned sig_ADRis0, sig_CMPL, sig_ADRL;

cm_t ff_CMA;
cm_t sig_CMSL;
cm_t ff_DCH;
cm_t ff_EAE;
cm_t ff_CJIT;
cm_t ff_EAE_P;
cm_t ff_ADSO;
cm_t ff_MBO;
cm_t ff_SUB;
cm_t ff_LI;
cm_t ff_IRI;
cm_t ff_AND;
cm_t ff_DEI;
cm_t ff_SM;
cm_t ff_AXS;
cm_t ff_KEY;
cm_t ff_PCO;
cm_t ff_MQO;
cm_t ff_ACO;
cm_t ff_ARO;
cm_t ff_EAE;
cm_t ff_EAE;
cm_t ff_TI;
cm_t ff_SKPI;
cm_t ff_DONE;
cm_t ff_CONT;
cm_t ff_EAE_R;
cm_t ff_PLUS1;
cm_t ff_MBI;
cm_t ff_ACI;
cm_t ff_ARI;
cm_t ff_PCI;
cm_t ff_MQI;
cm_t ff_EXT;

unsigned C, SEN, ff_PCOS, ff_ACOS, PV, LOT, ISZ, ff_CAL, REP, IOT, OP;
unsigned sig_CJIT_CAL_V_JMS, sig_CI17, sig_CO00;
unsigned ff_SAO;
unsigned KCT, KIOA3, KIOA4, KIOA5, ff_RUN;
unsigned INC_V_DCH, API_BK_RQ_B, PROG_SY, R12B;
unsigned ff_AUT_INX, BK_SYNC, ff_BK, ODD_ADDR, sig_ADDR10;
unsigned sig_KST, sig_KSP, sig_KCT, sig_KMT, sig_KIO, sig_KRI;
unsigned sig_KDP, sig_KDN, sig_KEX, sig_KEN;
unsigned sig_SW_SGL_INST, sig_SW_SGL_STP, sig_REPT;
unsigned ff_UM;
unsigned sig_NOSH, sig_SHL1, sig_SHL2, sig_SHR1, sig_SHR2;
unsigned sig_deltaMB, ff_MEM_STROBE;

static void pwr_clr_pos(void);
static void key_init_pos(void);
static void pk_clr(void);
static void pco_restore(void);
static void aco_restore(void);
static void zero_to_cma(void);
static void cm_clk(void);
static void cm_current(void);
static void cm_strobe_a(void);
static void cm_strobe_b(void);
static void cm_strobe_c(void);
static void cm_strobe_d(void);

static int power = 0;

void power_on(void)
{
  pwr_clr_pos();
  power = 1;
}

void power_off(void)
{
  VCD(RUN, ff_RUN = 0);
  MB = AC = AR = PC = MQ = 0;
  MA = IR = 0;
  power = 0;
  VAR(IR);
  VAR(MB);
  VAR(AC);
  VAR(AR);
  VAR(PC);
}

void cp_clk(void)
{
  static unsigned prev_KST = 0, prev_KCT = 0;
  if (!power)
    return;

  if ((!prev_KST && sig_KST) || (!prev_KCT && sig_KCT)) {
    KIOA3 = sig_KST || sig_KMT || sig_KIO;
    KIOA4 = sig_KST || sig_KMT || sig_KEN || sig_KDN;
    KIOA5 = sig_KMT || sig_KEX || sig_KDP || sig_KIO;
    if (KIOA3 || KIOA4 || KIOA5)
      key_init_pos();
  }

  if ((!prev_KST && sig_KST) || (!prev_KCT && sig_KCT))
    VCD(RUN, ff_RUN = 1);

  prev_KST = sig_KST;
  prev_KCT = sig_KCT;
}

static void ADR(void)
{
  sig_ADR = sig_A_BUS + sig_B_BUS + sig_CI17;
  sig_CO00 = sig_ADR >> 18;
  sig_ADR &= 0777777;
  if (sig_CMPL)
    sig_ADR ^= 0777777;
  sig_ADRis0 = sig_ADR == 0;
}

void clr(void)
{
  VCD(CLR, 1);

  VCD(MBO, ff_MBO = 0);
  ff_PLUS1 = 0;
  VCD(PCI, ff_PCI = 0);
  VCD(ACO, ff_ACO = 0);
  VCD(ACI, ff_ACI = 0);
  VCD(SAO, ff_SAO = 1);
  VCD(MBI, ff_MBI = 1);

  sig_A_BUS = 0;
  sig_B_BUS = 0;
  if (ff_SAO)
    sig_B_BUS = SA;
  sig_CI17 = ff_AUT_INX;
  ADR();
  
  sig_O_BUS = sig_ADR;
  if (ff_MBI)
    VCD(MB, MB = sig_O_BUS);
}

void cm_clk_pos(void)
{
  cm_clk();
}

static void cm_clk(void)
{
  VCD(CM_STROBE, 1);
  cm_current();
  cm_strobe_a();
  cm_strobe_b();
  cm_strobe_c();
  cm_strobe_d();

  if (sig_CJIT_CAL_V_JMS)
    VCD(PCI, ff_PCI = 1);
  sig_deltaMB = ff_PCO || ff_ARO || ff_CAL || ff_EXT || PV;
  if (sig_deltaMB && ff_SM && ff_RUN)
    VCD(MBI, ff_MBI = 1);

  sig_A_BUS = 0;
  if (ff_ADSO)
    sig_A_BUS = ADDR_SW;
  if (ff_ACO)
    sig_A_BUS = AC;
  if (ff_ARO)
    sig_A_BUS = AR;
  if (ff_PCO)
    sig_A_BUS = PC;
  if (ff_MQO)
    sig_A_BUS = MQ;

  sig_B_BUS = 0;
  if (ff_MBO)
    sig_B_BUS |= MB;
  if (ff_SUB)
    sig_B_BUS |= ~MB & 0777777;
  if (ff_AND)
    sig_B_BUS |= ~sig_A_BUS & 0777777;
  if (ff_SAO)
    sig_B_BUS |= SA;

  ADR();

  if (sig_NOSH) {
    sig_O_BUS = sig_ADR;
    sig_ADRL = sig_CO00;
  }
  if (sig_SHL1) {
    sig_O_BUS = ((sig_ADR << 1) & 0777777) | sig_CO00;
    sig_ADRL = sig_ADR >> 17;
  }
  if (sig_SHL2) {
    sig_O_BUS = ((sig_ADR << 2) & 0777777) | (sig_CO00 << 1) | (sig_ADR >> 17);
    sig_ADRL = (sig_ADR >> 16) & 1;
  }
  if (sig_SHR1) {
    sig_O_BUS = (sig_ADR >> 1) | (sig_CO00 << 17);
    sig_ADRL = sig_ADR & 1;
  }
  if (sig_SHR2) {
    sig_O_BUS = (sig_ADR >> 2) | (sig_CO00 << 16) | ((sig_ADR & 1) << 17);
    sig_ADRL = (sig_ADR >> 1) & 1;
  }
  if (ff_TI && ff_CAL)
    sig_O_BUS |= 020;

  if (ff_MBI)
    VCD(MB, MB = sig_O_BUS);
  if (ff_ACI)
    VCD(AC, AC = sig_O_BUS);
  if (ff_ARI)
    VCD(AR, AR = sig_O_BUS);
  if (ff_PCI)
    VCD(PC, PC = sig_O_BUS & 017777);
  if (ff_MQI)
    MQ = sig_O_BUS;
  if (ff_LI)
    LINK = sig_ADRL;
  printf("MB = %06o\nAC = %06o\nAR = %06o\nPC = %06o\n", MB, AC, AR, PC);
  printf("IR = %02o\n", IR << 1);
}

static void pwr_clr_pos(void)
{
  C = 0;
  SEN = 0;
  pk_clr();
  zero_to_cma();
}

static void key_init_pos(void)
{
  zero_to_cma();
}

static void sen(void)
{
  if (ff_PCO) {
    ff_PCOS = 1;
    if (KCT) {
      //...
      pco_restore();
    }
  }
  if (ff_ACO) {
    ff_ACOS = 1;
    if (KCT) {
      //...
      aco_restore();
    }
  }
}

static void pco_restore(void)
{
}

static void aco_restore(void)
{
}

static void pk_clr(void)
{
  PV = 0;
  ff_CMA &= ~073;
}

static void cm_current(void)
{
  unsigned ff_CMA0 = ff_CMA & 040;
  unsigned ff_CMA1 = ff_CMA & 020;
  unsigned ff_CMA2 = ff_CMA & 010;
  unsigned ff_CMA3 = ff_CMA & 004;
  unsigned ff_CMA4 = ff_CMA & 002;
  unsigned ff_CMA5 = ff_CMA & 001;
  unsigned address = ff_CMA0 | ff_CMA1;
  if (!ff_CMA0 && !ff_CMA1 && !ff_CMA2)
    address |= (KIOA3 << 2) | (KIOA4 << 1) | KIOA5;
  if (!REP)
    address |= ff_CMA2 | ff_CMA3 | ff_CMA4 | ff_CMA5;
  if (sig_ADDR10)
    address |= 010;
  if (ff_EXT)
    address |= (INC_V_DCH ? 4 : 0) | (API_BK_RQ_B ? 3 : 0);
  if (REP || (ff_CMA0 && ff_CMA1))
    address |= IR >> 1;
  if (REP)
    address |= 040;
  if ((PROG_SY && ff_EXT) || (ff_TI && !IR4 && !IR0 && !IR1))
    address |= 2;
  if ((ff_TI && IR4) || (ff_KEY && R12B) || (ff_DONE && BK_SYNC) || ODD_ADDR)
    address |= 1;
  printf("CMA/%02o\r\n", address);
  VCD(CMA, address);
  sig_CMSL = cm[address];
}

static void cm_strobe_a(void)
{
  ff_DCH = sig_CMSL & CM_DCH;
  if (ff_EAE || ff_LI)
    ff_LI = sig_CMSL & CM_LI;
  VCD(IRI, ff_IRI = sig_CMSL & CM_IRI);
  if (ff_IRI) {
    LOT = (SA >> 15) == 7;
    VCD(IR, IR = SA >> 13);
    PV = 1;
    ISZ = IR0 && !IR1 && !IR2 && IR3;
  }
  if (ff_IRI)
    ff_CAL = !IR0 && !IR1 && !IR2 && !IR3 && !ff_EXT;
  ff_AND = sig_CMSL & CM_AND;
  ff_DEI = sig_CMSL & CM_DEI;
  REP = (ff_IRI || ff_DEI) &&
    ((IR0 && IR1 && IR2) ||
     (IR0 && IR1 && !IR3) ||
     (IR0 && !IR2 && !IR3 && !IR4));
  VAR(REP);
  if (ff_DEI) {
    IR &= ~1;
    ff_CAL = 0;
  }

  VCD(SM, ff_SM = sig_CMSL & CM_SM);
  ff_AXS = sig_CMSL & CM_AXS;
  ff_KEY = sig_CMSL & CM_KEY;
}

static void cm_strobe_b(void)
{
  VCD(PCO, ff_PCO = sig_CMSL & CM_PCO);
  VCD(ACO, ff_ACO = sig_CMSL & CM_ACO);
  VCD(ARO, ff_ARO = sig_CMSL & CM_ARO);
  ff_MQO = sig_CMSL & CM_MQO;
  //IO_BUS_ON =
}

static void cm_strobe_c(void)
{
  ff_EAE = sig_CMSL & CM_EAE;
  ff_EAE_P = sig_CMSL & CM_EAE_P;
  ff_TI = sig_CMSL & CM_TI;
  ff_EAE = sig_CMSL & CM_EAE;
  ff_CMA = (sig_CMSL & CM_CMA) >> 7;

  ff_CJIT = sig_CMSL & CM_CJIT;
  sig_CJIT_CAL_V_JMS = ff_CJIT && !IR0 && !IR1 && !IR3;
  ff_ADSO = sig_CMSL & CM_ADSO;

  VCD(MBO, ff_MBO = sig_CMSL & CM_MBO);
  ff_SUB = sig_CMSL & CM_SUB;

  IOT = LOT && !IR3;
  OP = LOT && IR3 && !IR4;
  if (OP && (MB & 040) && !ff_UM)
    VCD(RUN, ff_RUN = 0);
  sig_SHR1 = OP && !MB07 && MB13;
  sig_SHL1 = OP && !MB07 && MB14;
  sig_SHR2 = OP && MB07 && MB13;
  sig_SHL2 = OP && MB07 && MB14;
  sig_NOSH = !sig_SHR1 && !sig_SHL1 && !sig_SHR2 && !sig_SHL2;

  ff_AUT_INX = ff_TI && IR4 && (MB & 017770) == 010;
}

static void zero_to_cma(void)
{
  ff_CMA &= 010;
}

static void thirteen_to_cma(void)
{
  ff_CMA |= 013;
}

static void cm_strobe_d(void)
{
  VCD(SAO, ff_SAO = 0);
  ff_SKPI = sig_CMSL & CM_SKPI;
  ff_DONE = sig_CMSL & CM_DONE;
  if (ff_DONE && (sig_KSP || sig_SW_SGL_INST))
    VCD(RUN, ff_RUN = 0);
  VCD(CONT, ff_CONT = sig_CMSL & CM_CONT);
  ff_EAE_R = sig_CMSL & CM_EAE_R;

  ff_PLUS1 = sig_CMSL & CM_PLUS1;
  sig_CI17 = !!ff_PLUS1;
  VCD(MBI, ff_MBI = sig_CMSL & CM_MBI);
  VCD(ACI, ff_ACI = sig_CMSL & CM_ACI);
  VCD(ARI, ff_ARI = sig_CMSL & CM_ARI);
  VCD(PCI, ff_PCI = sig_CMSL & CM_PCI);
  ff_MQI = sig_CMSL & CM_MQI;

  ff_EXT = sig_CMSL & CM_EXT;
  if (ff_EXT)
    ff_BK = 1;
}

static void zero_to_mbi(void)
{
  VCD(MBI, ff_MBI = 0);
}
