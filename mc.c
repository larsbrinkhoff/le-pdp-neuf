/* Memory controller. */

#include <stdio.h>
#include "defs.h"

static void ma_jam_word(void);
static void ma_jam_digit(void);

word_t MA;
word_t SA;

static word_t core[8192];

static void ma_jam_word(void)
{
  MA &= ~017760;
  MA |= MB & 017760;
}

static void ma_jam_digit(void)
{
  MA &= ~017;
  MA |= MB & 017;
}

void sync_clk(void)
{
  core[0100] = 0340123;
  core[0101] = 0600100;
  core[0123] = 0777001;

  ma_jam_digit();
  ma_jam_word();
  VAR(MA);
  VCD(SA, SA = core[MA]);
  printf("SA = %06o\n", SA);
}

//mem strobe = read data
//mem done

void mem_write(void)
{
  printf("Core %05o := %06o\n", MA, MB);
}
