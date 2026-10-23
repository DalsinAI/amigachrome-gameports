// Copyright (c) 2026 Dalsin Limited. GPL-2.0-only.
// Link with the pinned engine's HQFILE, LZ, ANIM_LE and BODY_LE objects.
// Compare every native cached byte with the engine's original load + fixup.
#include <SYSTEM/FILES.H>
#include <SYSTEM/HQFILE.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
extern "C" int LbaNativeCacheLoad(const char *, S32, void *, U32);
extern "C" void AnimFromLE(void *, S32);
extern "C" void ObjectBodyFromLE(void *, S32);
static FILE *handles[16];
extern "C" S32 OpenRead(const char *name) {
    for (int i=1;i<16;++i) if(!handles[i]) { handles[i]=fopen(name,"rb");return handles[i]?i:0; }
    return 0;
}
extern "C" S32 Close(S32 h) { int r=fclose(handles[h]);handles[h]=0;return r; }
extern "C" S32 Read(S32 h,void *p,U32 n) { return fread(p,1,n,handles[h]); }
extern "C" S32 Seek(S32 h,S32 n,ESeekOrigin o) { return fseek(handles[h],n,o==SEEK_FROM_START?SEEK_SET:o==SEEK_FROM_END?SEEK_END:SEEK_CUR); }
extern "C" U32 FileSize(const char *name) {
    FILE *f=fopen(name,"rb");if(!f)return 0;
    fseek(f,0,SEEK_END);long n=ftell(f);fclose(f);return n<0?0:n;
}
int main(int argc,char **argv) {
    if(argc!=2) { puts("usage: native-cache-probe DATA-DIR (set LBA2_NATIVE_CACHE)");return 2; }
    printf("cache=%s big_endian=%d\n",getenv("LBA2_NATIVE_CACHE")?getenv("LBA2_NATIVE_CACHE"):"(unset)", __BYTE_ORDER__==__ORDER_BIG_ENDIAN__);
    const char *banks[]={"ANIM.HQR","BODY.HQR","OBJFIX.HQR"};
    unsigned total=0;clock_t originalTime=0,cacheTime=0;
    for(int b=0;b<3;++b) {
        char path[1024];if(strlen(argv[1])>900)return 2;
        sprintf(path,"%s/%s",argv[1],banks[b]);
        int count=HQF_NbRes(path);if(count<=0)return 3;
        for(int i=0;i<count;++i) {
            clock_t start=clock();U32 size=HQF_Init(path,i);
            if(!size){HQF_Close();continue;}
            U8 *original=(U8*)malloc(size+1024),*cached=(U8*)malloc(size);
            if(!original||!cached)return 4;
            if(HQF_LoadClose(original)!=size)return 5;
            if(b==0)AnimFromLE(original,size);else ObjectBodyFromLE(original,size);
            originalTime+=clock()-start;start=clock();
            if(!LbaNativeCacheLoad(path,i,cached,size)) { printf("cache miss %s %d\n",banks[b],i);return 6; }
            cacheTime+=clock()-start;
            if(memcmp(original,cached,size)) {printf("MISMATCH %s %d\n",banks[b],i);return 7;}
            free(original);free(cached);++total;
        }
        printf("PASS %s %d entries\n",banks[b],count);fflush(stdout);
    }
    printf("PASS %u resources; original_ticks=%lu cache_ticks=%lu clocks_per_sec=%lu (includes first source verification)\n",total,(unsigned long)originalTime,(unsigned long)cacheTime,(unsigned long)CLOCKS_PER_SEC);
    return 0;
}
