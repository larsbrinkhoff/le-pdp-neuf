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
  static int first = 1;
  if (first) {
    core[0000] = 0777777;
    core[0100] = 0140000; //DMZ 0
    core[0101] = 0200000; //LAC 0
    core[0102] = 0340200; //TAD 200
    core[0103] = 0100170; //JMS 170
    core[0104] = 0340200; //TAD 0
    core[0105] = 0600102; //JMP 102
    core[0170] = 0000000;
    core[0171] = 0620170; //JMP I 170
    core[0200] = 0777001;
    first = 0;
  }

  ma_jam_digit();
  ma_jam_word();
  VAR(MA);
  VCD(SA, SA = core[MA]);
  core[MA] = 0;
  //printf("MA =  %05o\nSA = %06\n", MA, SA);
}

//mem strobe = read data
//mem done

void mem_write(void)
{
  //printf("Core: %05o := %06o\n", MA, MB);
  core[MA] = MB;
}
