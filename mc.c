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
  core[0000] = 0777777;
  core[0100] = 0140000;
  core[0101] = 0200000;
  core[0102] = 0340200;
  core[0103] = 0600102;
  core[0200] = 0777001;

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
  core[MA] = MB;
}
