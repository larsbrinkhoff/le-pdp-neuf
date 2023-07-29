#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <sys/select.h>
#include "defs.h"

static struct termios saved_termios;
static FILE *output;

static void (*normal_table[]) (void);
static void (*altmode_table[]) (void);
static void (*double_altmode_table[]) (void);

#define NBKPT (8+1)
#define NOBKPT (~0U)
static int trace;
static word_t breakpoints[NBKPT+2];
static int prefix = -1;
static int infix = -1;
static int q = -1;
static int *accumulator = &prefix;
static word_t dot = 0;
static void (**dispatch)(void) = normal_table;
static void (*typeout)(word_t data);
static int radix = 8;
static int crlf;
static int clear;
static char ch;
static word_t starta = 0100;

char *mnemonic[] ={
  "CAL", "CAL I", "DAC", "DAC I", "JMS", "JMS I", "DZM", "DZM I",
  "LAC", "LAC I", "XOR", "XOR I", "ADD", "ADD I", "TAD", "TAD I",
  "XCT", "XCT I", "ISZ", "ISZ I", "AND", "AND I", "SAD", "SAD I",
  "JMP", "JMP I", "EAE", "EAE", "IOT", "IOT", "OPR", "LAW"
};

static void push(unsigned *x)
{
  *x = 1;
  usleep(10000);
  *x = 0;
}

static void altmode(void)
{
  dispatch = altmode_table;
  clear = 0;
}

static void double_altmode(void)
{
  dispatch = double_altmode_table;
  clear = 0;
}

static void breakpoint(void)
{
  int i;
  if (prefix == -1) {
    if (infix != -1)
      breakpoints[infix] = NOBKPT;
    return;
  }
  if (infix != -1) {
    breakpoints[infix] = prefix;
    return;
  }
  for (i = 1; i < NBKPT; i++) {
    if (breakpoints[i] == NOBKPT) {
      breakpoints[i] = prefix;
      return;
    }
  }
  fprintf(output, "TOO MANY PTS? ");
}

static void clear_breakpoints(void)
{
  int i;
  for (i = 0; i < NBKPT+2; i++)
    breakpoints[i] = NOBKPT;
}

static void number(void)
{
  if (ch >= '0' && ch <= '7')
    ch -= '0';
  if (*accumulator == -1)
    *accumulator = ch;
  else
    *accumulator = 8 * *accumulator + ch;
  clear = 0;
}

static void quantity(void)
{
  prefix = q;
  clear = 0;
}

static void dis_eae(word_t data)
{
  fprintf(output, "EAE %o", data);
}

static void dis_iot(word_t data)
{
  fprintf(output, "IOT %o", data);
}

static void dis_opr(word_t data)
{
  fprintf(output, "OPR %o", data);
}

static void symbolic(word_t data)
{
  q = data;
  switch (data >> 13) {
  case 032: case 033:
    dis_eae(data & 037777); break;
  case 034: case 035:
    dis_iot(data & 037777); break;
  case 036:
    dis_opr(data & 017777); break;
  default:
    fprintf(output, "%s %o", mnemonic[data >> 13], data & 017777); break;
  }
  fprintf(output, "   ");
}

static void prnum(int data)
{
  int digit;
  digit = data % radix;
  data /= radix;
  if (data > 0)
    prnum(data);
  fprintf(output, "%o", digit);
}

static void equals(void)
{
  if (prefix == -1)
    prefix = q;
  q = prefix;
  prnum(prefix);
  fprintf(output, "   ");
  crlf = 0;
}

static void constant(word_t data)
{
  q = data;
  equals();
}

static void deposit(word_t addr, word_t data)
{
  ADDR_SW = dot = addr;
  DATA_SW = data;
  push(&sig_KDP);
}

static word_t examine(word_t addr)
{
  ADDR_SW = dot = addr;
  push(&sig_KEX);
  usleep(10000);
  return MB;
}

static word_t examine_next()
{
  push(&sig_KEN);
  usleep(10000);
  dot = MA;
  return MB;
}

static void carriagereturn(void)
{
  if (prefix == -1)
    return;
  deposit(dot, prefix);
}

static void slash(void)
{
  if (prefix != -1)
    dot = prefix;
  fprintf(output, "   ");
  typeout(examine(dot));
  fprintf(output, "   ");
  crlf = 0;
}

static void linefeed(void)
{
  carriagereturn();
  prefix = ++dot;
  fprintf(output, "\r\n%o/", prefix);
  fprintf(output, "   ");
  typeout(examine_next());
  fprintf(output, "   ");
  crlf = 0;
}

static void tab(void)
{
  carriagereturn();
  //XXXprefix = memory[dot + 1] | memory[dot + 2] << 8;
  fprintf(output, "\r\n%o/", prefix);
  slash();
}

static void caret(void)
{
  carriagereturn();
  dot--;
  prefix = dot;
  fprintf(output, "\r\n%o/", prefix);
  slash();
}

static void temporarily(void(*mode)(word_t), void(*fn)(void))
{
  void(*saved)(word_t) = typeout;
  typeout = mode;
  fn();
  typeout = saved;
}

static void rbracket(void)
{
  temporarily(constant, slash);
}

static void lbracket(void)
{
  temporarily(symbolic, slash);
}

static void control_c(void)
{
  fputs("\r\n", output);
}

static void control_g(void)
{
  fputs(" QUIT? ", output);
}

static void stopped(char *x)
{
  fprintf(output, "%o%s", PC, x);
  breakpoints[0] = NOBKPT;
  //XXXsymbolic(memory[pc]);
  crlf = 0;
}

#if 0 //XXX
static int control_z(void)
{
  struct timeval tv;
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fileno(stdin), &fds);
  tv.tv_sec = tv.tv_usec = 0;
  if (select(1, &fds, NULL, NULL, &tv) > 0) {
    int c = getchar();
    if (c == 007 || c == 032) {
      fputs("^Z\r\n", output);
      return 1;
    }
  }
  return 0;
}
#else
static void control_z(void)
{
  push(&sig_KSP);
  stopped(")   ");
}
#endif

static void proceed(void)
{
  fprintf(output, "\r\n");
  fflush(output);
  push(&sig_KCT);
}

static void go(void)
{
  if (prefix == -1)
    ADDR_SW = starta;
  else
    ADDR_SW = prefix;
  push(&sig_KST);
}

static void oneproceed(void)
{
  fprintf(output, "\r\n");
  fflush(output);
  sig_SW_SGL_INST = 1;
  push(&sig_KCT);
  sig_SW_SGL_INST = 0;
  stopped(">>");
}

static void next(void)
{
  fprintf(output, "\r\n");
  fflush(output);
  breakpoints[NBKPT] = PC + 1;
  breakpoints[NBKPT+1] = PC + 2;
  proceed();
  stopped(">>");
  breakpoints[NBKPT] = NOBKPT;
  breakpoints[NBKPT+1] = NOBKPT;
}

static void rubout(void)
{
}

static void error(void)
{
  fprintf(output, " OP? ");
}

static char buffer[100];

static char *line(void)
{
  char *p = buffer;
  char c;

  for (;;) {
    c = getchar();
    if (c == 015)
      return buffer;
    *p++ = c;
    fputc(c, output);
  }
}

/* Presently my soul grew stronger: hesitating then no longer
I decided that I would respond to this strange program's call;
TECO, which I then attended, to my soul more strength extended;
With ^Z I ascended, going to my DDT --
$$V I typed, and answered soon my DDT -- TECO there, and that was all! */

static void teco(void)
{
  fprintf(output, "\r\n* TECO");
}

/* Again I went up to my HACTRN while cold shivers up my back ran
$$V I typed, my jobs now once more to display.
Only TECO was there listed; though my trembling heart resisted
Yet I willed my hand, insisted, $J to quickly type --
To answer this bold query DDT did hesitantly type
A ghostly "FOOBAR$J" */

static void foobar(void)
{
  fprintf(output, "\r\nFOOBAR$J");
}

static void load(void)
{
  char *file;
  FILE *f;

  if (prefix == -1) {
    fprintf(output, "\r\nMust supply a start address");
    return;
  }

  fputc(' ', output);
  file = line();
  f = fopen(file, "rb");
  if (f == NULL) {
    fprintf(output, "\r\n%s - file not found", file);
    return;
  }
  ptr_mount(f);
  ADDR_SW = prefix;
  push(&sig_KRI);
  fclose(f);
}

static void dump(void)
{
  //XXX
}

static void zero(void)
{
  int i;
  if (prefix != -1)
    ADDR_SW = prefix;
  else
    ADDR_SW = 0;
  DATA_SW = 0;
  push(&sig_KDP);
  for (i = ADDR_SW + 1; i < 8192; i++)
    push(&sig_KDN);
}

static void login(void)
{
  fprintf(output, "Welcome!\r\n");
}

static void logout(void)
{
  tcsetattr(fileno(stdin), TCSAFLUSH, &saved_termios);
  fprintf(output, "\r\n");
  exit(0);
}

static void period(void)
{
  prefix = dot;
  clear = 0;
}

static void altmode_period(void)
{
  prefix = PC;
  clear = 0;
}

static void altmode_s(void)
{
  typeout = symbolic;
}

static void altmode_c(void)
{
  typeout = constant;
}

static void altmode_d(void)
{
  radix = 10;
  typeout = constant;
}

static void altmode_o(void)
{
  radix = 8;
  typeout = constant;
}

static void space(void)
{
}

static void colon(void)
{
  char *command = line();
  //XXX
}

static void(*normal_table[])(void) = {
  error,                  //^@
  error,                  //^A
  error,                  //^B
  control_c,              //^C
  error,                  //^D
  error,                  //^E
  error,                  //^F
  control_g,              //^G
  error,                  //^H
  tab,                    //^I
  linefeed,               //^J
  error,                  //^K
  error,                  //^L
  carriagereturn,         //^M
  oneproceed,             //^N
  error,                  //^O
  error,                  //^P
  error,                  //^Q
  error,                  //^R
  error,                  //^S
  error,                  //^T
  error,                  //^U
  error,                  //^V
  error,                  //^W
  error,                  //^X
  error,                  //^Y
  control_z,              //^Z
  altmode,                //^[
  error,                  //control-backslash
  error,                  //^]
  error,                  //^^
  error,                  //^_
  space,                  // 
  error,                  //!
  error,                  //"
  error,                  //#
  error,                  //$
  error,                  //%
  error,                  //&
  error,                  //'
  error,                  //(
  error,                  //)
  error,                  //*
  error,                  //+
  error,                  //,
  error,                  //-
  period,                 //.
  slash,                  ///
  number,                 //0
  number,                 //1
  number,                 //2
  number,                 //3
  number,                 //4
  number,                 //5
  number,                 //6
  number,                 //7
  number,                 //8
  number,                 //9
  colon,                  //:
  error,                  //;
  error,                  //<
  equals,                 //=
  error,                  //>
  error,                  //?
  error,                  //@
  number,                 //A
  number,                 //B
  number,                 //C
  number,                 //D
  number,                 //E
  number,                 //F
  error,                  //G
  error,                  //H
  error,                  //I
  error,                  //J
  error,                  //K
  error,                  //L
  error,                  //M
  error,                  //N
  error,                  //O
  error,                  //P
  error,                  //Q
  error,                  //R
  error,                  //S
  error,                  //T
  error,                  //U
  error,                  //V
  error,                  //W
  error,                  //X
  error,                  //Y
  error,                  //Z
  rbracket,               //[
  error,                  //backslash
  lbracket,               //]
  caret,                  //^
  error,                  //_
  error,                  //`
  number,                 //a
  number,                 //b
  number,                 //c
  number,                 //d
  number,                 //e
  number,                 //f
  error,                  //g
  error,                  //h
  error,                  //i
  error,                  //j
  error,                  //k
  error,                  //l
  error,                  //m
  error,                  //n
  error,                  //o
  error,                  //p
  error,                  //q
  error,                  //r
  error,                  //s
  error,                  //t
  error,                  //u
  error,                  //v
  error,                  //w
  error,                  //x
  error,                  //y
  error,                  //z
  error,                  //{
  error,                  //|
  error,                  //}
  error,                  //~
  rubout,                 //^?
};

static void(*altmode_table[])(void) = {
  error,                  //^@
  error,                  //^A
  error,                  //^B
  error,                  //^C
  error,                  //^D
  error,                  //^E
  error,                  //^F
  error,                  //^G
  error,                  //^H
  error,                  //^I
  error,                  //^J
  error,                  //^K
  error,                  //^L
  error,                  //^M
  next,                   //^N
  error,                  //^O
  error,                  //^P
  error,                  //^Q
  error,                  //^R
  error,                  //^S
  error,                  //^T
  error,                  //^U
  error,                  //^V
  error,                  //^W
  error,                  //^X
  error,                  //^Y
  error,                  //^Z
  double_altmode,         //^[
  error,                  //control-backslash
  error,                  //^]
  error,                  //^^
  error,                  //^_
  error,                  // 
  error,                  //!
  error,                  //"
  error,                  //#
  error,                  //$
  error,                  //%
  error,                  //&
  error,                  //'
  error,                  //(
  error,                  //)
  error,                  //*
  error,                  //+
  error,                  //,
  error,                  //-
  altmode_period,         //.
  error,                  ///
  error,                  //0
  error,                  //1
  error,                  //2
  error,                  //3
  error,                  //4
  error,                  //5
  error,                  //6
  error,                  //7
  error,                  //8
  error,                  //9
  error,                  //:
  error,                  //;
  error,                  //<
  error,                  //=
  error,                  //>
  error,                  //?
  error,                  //@
  error,                  //A
  breakpoint,             //B
  altmode_c,              //C
  altmode_d,              //D
  error,                  //E
  error,                  //F
  go,                     //G
  error,                  //H
  error,                  //I
  foobar,                 //J
  error,                  //K
  load,                   //L
  error,                  //M
  error,                  //N
  altmode_o,              //O
  proceed,                //P
  quantity,               //Q
  error,                  //R
  altmode_s,              //S
  error,                  //T
  login,                  //U
  error,                  //V
  error,                  //W
  error,                  //X
  dump,                   //Y
  error,                  //Z
  error,                  //[
  error,                  //backslash
  error,                  //]
  error,                  //^
  error,                  //_
  error,                  //`
  error,                  //a
  breakpoint,             //b
  altmode_c,              //c
  altmode_d,              //d
  error,                  //e
  error,                  //f
  go,                     //g
  error,                  //h
  error,                  //i
  foobar,                 //j
  error,                  //k
  load,                   //L
  error,                  //m
  error,                  //n
  altmode_o,              //o
  proceed,                //p
  quantity,               //q
  error,                  //r
  altmode_s,              //s
  error,                  //t
  login,                  //u
  error,                  //v
  error,                  //w
  error,                  //x
  dump,                   //y
  error,                  //z
  error,                  //{
  error,                  //|
  error,                  //}
  error,                  //~
  error,                  //^?
};

static void(*double_altmode_table[])(void) = {
  error,                  //^@
  error,                  //^A
  error,                  //^B
  error,                  //^C
  error,                  //^D
  error,                  //^E
  error,                  //^F
  error,                  //^G
  error,                  //^H
  error,                  //^I
  error,                  //^J
  error,                  //^K
  error,                  //^L
  error,                  //^M
  error,                  //^N
  error,                  //^O
  error,                  //^P
  error,                  //^Q
  error,                  //^R
  error,                  //^S
  error,                  //^T
  error,                  //^U
  error,                  //^V
  error,                  //^W
  error,                  //^X
  error,                  //^Y
  error,                  //^Z
  error,                  //^[
  error,                  //control-backslash
  error,                  //^]
  error,                  //^^
  error,                  //^_
  error,                  // 
  error,                  //!
  error,                  //"
  error,                  //#
  error,                  //$
  error,                  //%
  error,                  //&
  error,                  //'
  error,                  //(
  error,                  //)
  error,                  //*
  error,                  //+
  error,                  //,
  error,                  //-
  error,                  //.
  error,                  ///
  error,                  //0
  error,                  //1
  error,                  //2
  error,                  //3
  error,                  //4
  error,                  //5
  error,                  //6
  error,                  //7
  error,                  //8
  error,                  //9
  error,                  //:
  error,                  //;
  error,                  //<
  error,                  //=
  error,                  //>
  error,                  //?
  error,                  //@
  error,                  //A
  clear_breakpoints,      //B
  error,                  //C
  error,                  //D
  error,                  //E
  error,                  //F
  error,                  //G
  error,                  //H
  error,                  //I
  error,                  //J
  error,                  //K
  load,                   //L
  error,                  //M
  error,                  //N
  error,                  //O
  error,                  //P
  error,                  //Q
  error,                  //R
  error,                  //S
  error,                  //T
  logout,                 //U
  teco,                   //V
  error,                  //W
  error,                  //X
  dump,                   //Y
  zero,                   //Z
  error,                  //[
  error,                  //backslash
  error,                  //]
  error,                  //^
  error,                  //_
  error,                  //`
  error,                  //a
  clear_breakpoints,      //b
  error,                  //c
  error,                  //d
  error,                  //e
  error,                  //f
  error,                  //g
  error,                  //h
  error,                  //i
  error,                  //j
  error,                  //k
  load,                   //l
  error,                  //m
  error,                  //n
  error,                  //o
  error,                  //p
  error,                  //q
  error,                  //r
  error,                  //s
  error,                  //t
  logout,                 //u
  teco,                   //v
  error,                  //w
  error,                  //x
  dump,                   //y
  zero,                   //z
  error,                  //{
  error,                  //|
  error,                  //}
  error,                  //~
  error,                  //^?
};

static void cbreak(void)
{
  struct termios t;

  if (tcgetattr(fileno(stdin), &t) == -1)
    return;

  saved_termios = t;
  t.c_lflag &= ~(ICANON | ECHO);
  t.c_lflag &= ~ISIG;
  t.c_iflag &= ~ICRNL;
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;

  atexit(logout);

  if (tcsetattr(fileno(stdin), TCSAFLUSH, &t) == -1)
    return;
}

static void echo(char c)
{
  if (c == 015)
    ;
  else if (c == 033)
    fprintf(output, "$");
  else if (c < 32)
    fprintf(output, "^%c", c + '@');
  else if (c == 0177)
    fprintf(output, "^?");
  else
    fprintf(output, "%c", c);
  fflush(output);
}

static char upcase(char c)
{
  if (c >= 'a' && c <= 'z')
    c &= ~040;
  return c;
}

static void key(char c)
{
  void(**before)(void) = dispatch;
  if (c & ~0177)
    return;
  c = upcase(c);
  echo(c);
  clear = 1;
  crlf = 1;
  ch = c;
  dispatch[(int)c]();
  if (dispatch == before)
    dispatch = normal_table;
  if (clear) {
    prefix = -1;
    infix = -1;
    accumulator = &prefix;
    if (crlf)
      fprintf(output, "\r\n");
  }
  fflush(output);
}

static void * thread(void *arg)
{
  for (;;)
    key(getchar());
  return 0;
}

void ddt(void)
{
  pthread_t th;
  output = stdout;
  cbreak();
  trace = 0;
  typeout = symbolic;
  radix = 8;
  prefix = infix = -1;
  clear_breakpoints();
  pthread_create(&th, NULL, thread, NULL);
}

void panel(void)
{
  power_on();
  ddt();
}

void quit(int x)
{
  exit(0);
}
