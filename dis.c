#include <stdio.h>
#include <stdlib.h>

static char line[100];
static unsigned long cm[64];

static const char *bit[] = {
  /* 00 */ "SUB",
  /* 01 */ "EAE",
  /* 02 */ "EAE-R",
  /* 03 */ "MBO",
  /* 04 */ "MBI",
  /* 05 */ "SKPI",
  /* 06 */ "ACO",
  /* 07 */ "ACI",
  /* 08 */ "IRI",
  /* 09 */ "ARO",
  /* 10 */ "ARI",
  /* 11 */ "CJIT",
  /* 12 */ "PCO",
  /* 13 */ "PCI",
  /* 14 */ "EAE-P",
  /* 15 */ "MQO",
  /* 16 */ "MQI",
  /* 17 */ "DEI",
  /* 18 */ "LI",
  /* 19 */ "AND",
  /* 20 */ "TI",
  /* 21 */ "DONE",
  /* 22 */ "AXS",
  /* 23 */ "CMA 0",
  /* 24 */ "CMA-1",
  /* 25 */ "CMA 2",
  /* 26 */ "CMA-3",
  /* 27 */ "CMA 4",
  /* 28 */ "CMA-5",
  /* 29 */ "EXT",
  /* 30 */ "CONT",
  /* 31 */ "+1",
  /* 32 */ "SM",
  /* 33 */ "ADSO",
  /* 34 */ "KEY",
  /* 35 */ "DCH",
};

static void skip (int n)
{
  while (n--)
    fgets (line, sizeof line, stdin);
}

static void binary(unsigned long word)
{
  int i;
  for (i = 0; i < 36; i++) {
    putchar ((word & 0400000000000UL) ? '1' : '0');
    word <<= 1;
  }
}

static void fields(unsigned long word)
{
  int i;
  for (i = 0; i < 36; i++) {
    if (word & 0400000000000UL)
      printf ("%s ", bit[i]);
    word <<= 1;
  }
}

static int cma(unsigned long word)
{
  return (word >> 7) & 077;
}

int main (void)
{
  unsigned long word, x;
  int i, j, index;
  skip(43);

  while (!feof(stdin)) {
    fgets (line, sizeof line, stdin);

    line[2] = 0;
    index = strtol(line, NULL, 8);
    
    word = 0;
    x = 0400000000000UL;
    for (i = 3; i < 74; i += 2) {
      if (line[i] == '/')
        word |= x;
      x >>= 1;
    }

    cm[index] = word;
  }

  for (i = 0; i < 64; i++) {
    printf ("%02o:", i);
    binary(cm[i]);
    printf (" ");
    fields(cm[i] & ~017650ULL);
    fields(cm[i] & 050ULL);
    printf ("CMA/%02o\n", cma(cm[i]));
  }
}
