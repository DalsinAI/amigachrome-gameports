/* Isolated FS-UAE lab helper; never installed to a user instance.
 * Copyright (c) 2026 AmigaChrome Team. SPDX-License-Identifier: MIT */
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdio.h>
int main(void) {
    const ULONG start=0x40000000UL, bytes=0x10000000UL, end=start+bytes;
    struct MemHeader *h;
    Forbid();
    for(h=(struct MemHeader*)SysBase->MemList.lh_Head;h->mh_Node.ln_Succ;h=(struct MemHeader*)h->mh_Node.ln_Succ) {
        if((ULONG)h->mh_Lower < end && (ULONG)h->mh_Upper > start) {
            Permit(); puts("Refusing: candidate region overlaps registered memory"); return 2;
        }
    }
    Permit();
    AddMemList(bytes,MEMF_PUBLIC|MEMF_FAST,0,(APTR)start,"Enigma isolated Z3 lab RAM");
    printf("Available fast RAM after registration: %lu\n",(unsigned long)AvailMem(MEMF_FAST));
    return 0;
}
