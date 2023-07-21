/* Use a Blincolnlights-18 panel. */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <pigpio.h>
#include "defs.h"

/*
Lights:

MB 0-17
REG 0-17
LINK

IR 0-4

PS ACTIVE 0-8
PIE
DATA

PGM STOP: lights[2] 000004

ERROR
REPT
SINGLE INST
SINGLE STEP
CLK
EXD
PRTCT
*/

/*
Switches:

REGISTER DISPLAY:
EN
ACD
ARD
PCD
MQD
EAE D
RDR D
TTI D
STATUS D
API D
DPY D
IO ADDR D

REPT SPEED: OFF 1 2 3 4 5

READ-IN (KRI): switches[1] 010000
IO RESET (KIO): switches[1] 004000
DEPOSIT NEXT (DPN)
DEPOSIT (KDP): switches[1] 020000
EXAMINE NEXT (KEN)
EXAMINE (KEX): switches[1] 040000
CONTINUE (KCT): switches[1] 100000
STOP (KSP): switches[1] 200000
START (KST): switches[1] 400000

DATA SW 0-17: switches[0] 777777
ADDR SW 3-17

SW PARITY
SW REPT: switches[1] 000004
SW SGL INST: switches[1] 000010
SW SGK STP: switches[1] 000020
SW CLK
SW EXD
SW PRTCT
*/

#define ELTS(X) (int)(sizeof X / sizeof X[0])

/* Blincolnlights, first row is upper data indicators. */
/* Blincolnlights, second row is lower data indicators. */
/* Blincolnlights, third row. */
#define B18_LIGHT_AC2    0400000
#define B18_LIGHT_PC     0200000
#define B18_LIGHT_MB     0100000
#define B18_LIGHT_TBR    0040000
#define B18_LIGHTS_lo    0740000
#define B18_LIGHT_AC     0020000
#define B18_LIGHT_MA     0010000
#define B18_LIGHT_Flags  0004000
#define B18_LIGHT_TAC    0002000
#define B18_LIGHTS_hi    0036000
#define B18_LIGHT_Run    0001000
#define B18_LIGHT_Flag1  0000400
#define B18_LIGHT_Flag2  0000200
#define B18_LIGHT_Flag3  0000100
#define B18_LIGHT_Power  0000040
#define B18_LIGHT_SingS  0000020
#define B18_LIGHT_SingI  0000010
#define B18_LIGHT_Repeat 0000004
#define B18_LIGHT_Spare1 0000002
#define B18_LIGHT_Spare2 0000001

/* Switches, first row is data/test. */
/* Switches, second row. */
#define B18_SW_Start     0400000
#define B18_SW_Stop      0200000
#define B18_SW_Cont      0100000
#define B18_SW_Examine   0040000
#define B18_SW_Deposit   0020000
#define B18_SW_Read_in   0010000
#define B18_SW_blank     0004000
#define B18_SW_Mod       0002000
#define B18_SW_Select_hi 0001000
#define B18_SW_Load_hi   0000400
#define B18_SW_Select_lo 0000200
#define B18_SW_Load_lo   0000100
#define B18_SW_Power     0000040
#define B18_SW_SingS     0000020
#define B18_SW_SingI     0000010
#define B18_SW_Repeat    0000004
#define B18_SW_Spare1    0000002
#define B18_SW_Spare2    0000001

static atomic_int looping;
static atomic_int lights[3];
static atomic_int switches[2];
static int osw;

static int light_row[] = { 2, 3, 4 };
static int switch_row[] = { 17, 27 };
static int column_pin[] =
  { 26, 19, 13, 6, 5, 11, 14, 15, 18, 23, 24, 25, 8, 7, 12, 16, 20, 21 };

static void setup(void)
{
  int i;
  for(i = 0; i < ELTS(light_row); i++)
    gpioSetMode(light_row[i], PI_OUTPUT);
  for(i = 0; i < ELTS(switch_row); i++)
    gpioSetMode(switch_row[i], PI_OUTPUT);
  for (i = 0; i < ELTS(column_pin); i++)
    gpioSetPullUpDown(column_pin[i], PI_PUD_UP);
}

static void columns(unsigned mode)
{
  int i;
  for (i = 0; i < ELTS(column_pin); i++)
    gpioSetMode(column_pin[i], mode);
}

static void write_lights(void)
{
  int i, j;
  if ((switches[1] & B18_SW_Power) == 0)
    return;
  columns(PI_OUTPUT);
  for (i = 0; i < ELTS(light_row); i++) {
      gpioWrite(light_row[i], 1);
      for (j = 0; j < ELTS(column_pin); j++)
        gpioWrite(column_pin[j], (~lights[i] >> j) & 1);
      usleep(1000);
      gpioWrite(light_row[i], 0);
    }
}

static void read_switches(void)
{
  int i, j, data;
  columns(PI_INPUT);
  for (i = 0; i < ELTS(switch_row); i++) {
    gpioWrite(switch_row[i], 0);
    usleep(2);
    data = 0;
    for (j = 0; j < ELTS(column_pin); j++)
      data |= !gpioRead(column_pin[j]) << j;
    switches[i] = data;
    gpioWrite(switch_row[i], 1);
  }
}

static void frob(void)
{
  int on, off;

  lights[2] =
    (ff_RUN ? B18_LIGHT_Run : 0) |
    ((switches[1] & B18_SW_Power) ? B18_LIGHT_Power : 0) |
    ((switches[1] & B18_SW_SingI) ? B18_LIGHT_SingI : 0) |
    ((switches[1] & B18_SW_SingS) ? B18_LIGHT_SingS : 0) |
    (lights[2] & (B18_LIGHTS_hi | B18_LIGHTS_lo));

  sig_KST = switches[1] & B18_SW_Start;
  sig_KSP = switches[1] & B18_SW_Stop;
  sig_KCT = switches[1] & B18_SW_Cont;
  sig_KRI = switches[1] & B18_SW_Read_in;
  sig_KIO = switches[1] & B18_SW_blank;
  if (switches[1] & B18_SW_Mod) {
    sig_KEN = switches[1] & B18_SW_Examine;
    sig_KDN = switches[1] & B18_SW_Deposit;
  } else {
    sig_KEX = switches[1] & B18_SW_Examine;
    sig_KDP = switches[1] & B18_SW_Deposit;
  }
  sig_SW_SGL_INST = switches[1] & B18_SW_SingI;
  sig_SW_SGL_STP = switches[1] & B18_SW_SingS;

  on = switches[1] & (switches[1] ^ osw);
  off = (~switches[1]) & (switches[1] ^ osw);

  if (on & B18_SW_Power)
    power_on();
  if (off & B18_SW_Power)
    power_off();

  if (on & B18_SW_Select_hi) {
    lights[2] = (lights[2] & ~B18_LIGHTS_hi) |
      (((lights[2] & B18_LIGHTS_hi) >> 1) & B18_LIGHTS_hi);
    if ((lights[2] & B18_LIGHTS_hi) == 0)
      lights[2] |= B18_LIGHT_AC;
  }

  if (on & B18_SW_Select_lo) {
    lights[2] = (lights[2] & ~B18_LIGHTS_lo) |
      (((lights[2] & B18_LIGHTS_lo) >> 1) & B18_LIGHTS_lo);
    if ((lights[2] & B18_LIGHTS_lo) == 0)
      lights[2] |= B18_LIGHT_AC2;
  }

  if (on & B18_SW_Start) {
    ADDR_SW = switches[0];
  }

  if (on & B18_SW_Examine) {
    ADDR_SW = switches[0];
  }

  if (lights[2] & B18_LIGHT_AC)
    lights[0] = AC;
  if (lights[2] & B18_LIGHT_MA)
    lights[0] = MA;
  if (lights[2] & B18_LIGHT_Flags)
    lights[0] = 0;
  if (lights[2] & B18_LIGHT_TAC)
    lights[0] = ADDR_SW;
  if (lights[2] & B18_LIGHT_AC2)
    lights[1] = AR;
  if (lights[2] & B18_LIGHT_PC)
    lights[1] = (IR << 13) | PC;
  if (lights[2] & B18_LIGHT_MB)
    lights[1] = MB;
  if (lights[2] & B18_LIGHT_TBR)
    lights[1] = switches[0];

  if (switches[1] & B18_LIGHT_Spare2) {
    lights[0] = sig_CMSL >> 18;
    lights[1] = sig_CMSL & 0777777;
    lights[2] = (lights[2] & ~077) | ff_CMA;
  }
}

static void * thread(void *data)
{
  setup();
  lights[2] = B18_LIGHT_AC | B18_LIGHT_PC;
  while (looping) {
    osw = switches[1];
    write_lights();
    read_switches();
    frob();
  }
  gpioTerminate();
  exit(0);
  return NULL;
}

void panel(void)
{
  pthread_t th;
  if (gpioInitialise() < 0)
    exit(1);
  looping = 1;
  pthread_create(&th, NULL, thread, NULL);
}

void quit(int x)
{
  looping = 0;
}
