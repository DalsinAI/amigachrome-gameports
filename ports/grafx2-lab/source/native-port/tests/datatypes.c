/* GPL-2.0-only. Actual bridge with DataTypes/DOS call mocks; not OS qualification. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define _NO_INLINE
#define __USE_NEW_TIMEVAL__
#define USE_AMIGA_NATIVE
#include "amigadatatype.c"
signed char File_error;
static struct BitMapHeader header;
static int no_writer, fail_write, fail_save, fail_commit, writes, objects, closes;
static byte pixels[8];
struct Library *OpenLibrary(CONST_STRPTR n,ULONG v) {static struct Library lib;return &lib;}
LONG IoErr(void) { return 0; }
void CloseLibrary(struct Library *l) {closes++;}
struct Task *FindTask(CONST_STRPTR name) {return (struct Task *)0x1234;}
Object *NewDTObjectA(STRPTR name,const struct TagItem *tags)
{
 if(no_writer&&!name)return NULL;
 memset(&header,0,sizeof(header));header.bmh_Width=2;header.bmh_Height=1;header.bmh_Depth=32;
 objects++;return (Object *)0x100;
}
void DisposeDTObject(Object *o) {objects--;}
ULONG GetDTAttrsA(Object *o,const struct TagItem *tags)
{
 ULONG count=0;
 for(;tags->ti_Tag!=TAG_DONE;tags++)
 {
  ULONG tag=tags->ti_Tag,ptr=tags->ti_Data;
  if(tag==PDTA_BitMapHeader) *(struct BitMapHeader **)ptr=&header;
  else if(tag==PDTA_CRegs) *(ULONG **)ptr=NULL;
  else if(tag==PDTA_ModeID || tag==PDTA_NumColors) *(ULONG *)ptr=0;
  count++;
 }
 return count;
}
ULONG SetDTAttrsA(Object *o,struct Window *w,struct Requester *r,const struct TagItem *tags){return 1;}
ULONG DoMethodA(Object *o,Msg msg)
{
 struct pdtBlitPixelArray *p=(void *)msg;
 assert(p->pbpa_PixelData);assert(p->pbpa_Width==2);assert(p->pbpa_Height==1);
 if(p->MethodID==PDTM_WRITEPIXELARRAY)
 {if(fail_write)return 0;memcpy(pixels,p->pbpa_PixelData,8);writes++;return 1;}
 assert(p->MethodID==PDTM_READPIXELARRAY);
 const byte sample[8]={255,11,22,33,255,44,55,66};memcpy(p->pbpa_PixelData,sample,8);return 1;
}
ULONG SaveDTObjectA(Object *o,struct Window *w,struct Requester *r,CONST_STRPTR file,ULONG mode,LONG icon,const struct TagItem *tags)
{
 if(fail_save)return 0;assert(mode==DTWM_RAW);FILE *f=fopen((const char *)file,"wb");assert(f);fwrite("\211PNG\r\n\032\n",1,8,f);fclose(f);return 1;
}
BPTR Lock(CONST_STRPTR path,LONG mode){return access((const char *)path,F_OK)==0?1:0;}
void UnLock(BPTR lock){}
LONG Rename(CONST_STRPTR from,CONST_STRPTR to)
{
 if(fail_commit && strstr((const char *)from,".tmp"))return 0;
 return rename((const char *)from,(const char *)to)==0;
}
LONG DeleteFile(CONST_STRPTR p){return unlink((const char *)p)==0;}
char *Filepath_append_to_dir(const char *dir,const char *name){char *p=malloc(strlen(dir)+strlen(name)+2);sprintf(p,"%s/%s",dir,name);return p;}
unsigned long File_length(const char *path){return 10;}
void Pre_load(T_IO_Context *c,short w,short h,long n,int f,enum PIXEL_RATIO ratio,byte bpp){assert(File_error==0);c->Width=w;c->Height=h;}
void Set_pixel(T_IO_Context *c,short x,short y,byte p){assert(0);}
void Set_pixel_24b(T_IO_Context *c,short x,short y,byte r,byte g,byte b){assert(r==(x?44:11));assert(g==(x?55:22));assert(b==(x?66:33));}
byte Get_pixel(T_IO_Context *c,short x,short y){return x;}
void GFX2_Log(GFX2_Log_priority_T p,const char *fmt,...){}
int GFX2_MessageBox(const char *text,const char *caption,unsigned int type){return 1;}
static void original(const char *p){FILE *f=fopen(p,"wb");assert(f);fputs("ORIGINAL",f);fclose(f);}
static void check(const char *p,const char *expected){char b[32]={0};FILE *f=fopen(p,"rb");assert(f);fread(b,1,31,f);fclose(f);assert(!strcmp(b,expected));}
int main(int argc,char **argv)
{
 T_IO_Context c={0};char path[512];assert(argc==2);snprintf(path,sizeof(path),"%s/result.png",argv[1]);
 c.File_directory=argv[1];c.File_name="result.png";c.Width=2;c.Height=1;c.Palette[0].R=17;c.Palette[1].G=29;c.Background_transparent=1;c.Transparent_color=1;
 original(path);Save_AmigaPNG(&c);assert(!File_error&&!objects&&writes==1);check(path,"\211PNG\r\n\032\n");assert(pixels[0]==255&&pixels[1]==17&&pixels[4]==0&&pixels[6]==29);
 for(int mode=0;mode<4;mode++)
 {
  original(path);no_writer=mode==0;fail_write=mode==1;fail_save=mode==2;fail_commit=mode==3;
  Save_AmigaPNG(&c);assert(File_error&&!objects);check(path,"ORIGINAL");
 }
 no_writer=fail_write=fail_save=fail_commit=0;Load_AmigaDataType(&c);assert(!File_error&&!objects);
 GFX2_AmigaDataTypesClose();assert(closes==1);unlink(path);
 puts("datatypes: import, ARGB/alpha export, missing writer, write/save failures and commit rollback passed");return 0;
}
