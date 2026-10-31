/* GPL-2.0-only: exercise the actual editor palette rounding on the target. */
#include <stdio.h>
#include "struct.h"
extern int RGB_scale;
extern byte Round_palette_component(byte);
extern void Set_palette_Gamma(int);
int main(void)
{
  int gamma,c,pass,bad=0;
  RGB_scale=256;
  for(gamma=5;gamma<=25;gamma+=5)
  {
    Set_palette_Gamma(gamma);
    for(c=0;c<256;c++)
    {
      byte value=c;
      for(pass=0;pass<32;pass++)value=Round_palette_component(value);
      if(value!=c)bad++;
    }
  }
  RGB_scale=16;
  if(Round_palette_component(0)!=0||Round_palette_component(255)!=255)bad++;
  printf("PALETTE IDENTITY: %s (%d failures)\n",bad?"FAIL":"PASS",bad);
  return bad?20:0;
}
