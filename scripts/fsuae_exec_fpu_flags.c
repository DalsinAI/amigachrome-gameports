/* SPDX-License-Identifier: MIT -- diagnostic of the published ExecBase ABI. */
#include <stdio.h>
int main(void){volatile unsigned char* base; __asm__ volatile("move.l 4.w,%0" : "=a"(base));unsigned flags=*(volatile unsigned short*)(base+296);printf("ExecBase AttnFlags = 0x%04x; AFF_68881=%u AFF_FPU40=%u\n",flags,!!(flags&16),!!(flags&64));return 0;}
