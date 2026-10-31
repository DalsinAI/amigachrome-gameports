/* GPL-2.0-only: run the actual GrafX2 bridge with installed system DataTypes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "loadsave.h"
#include "io.h"
#include "gfx2log.h"
signed char File_error;
static byte result[12];
void Load_AmigaDataType(T_IO_Context *);
void Save_AmigaPNG(T_IO_Context *);
void GFX2_AmigaDataTypesClose(void);
void Pre_load(T_IO_Context *c,short w,short h,long size,int format,enum PIXEL_RATIO ratio,byte bpp)
{if(w!=2||h!=2){File_error=1;return;}c->Width=w;c->Height=h;}
void Set_pixel(T_IO_Context *c,short x,short y,byte i)
{T_Components p=c->Palette[i];int n=(y*2+x)*3;result[n]=p.R;result[n+1]=p.G;result[n+2]=p.B;}
void Set_pixel_24b(T_IO_Context *c,short x,short y,byte r,byte g,byte b)
{int n=(y*2+x)*3;result[n]=r;result[n+1]=g;result[n+2]=b;}
byte Get_pixel(T_IO_Context *c,short x,short y){return (x+y)&1;}
char *Filepath_append_to_dir(const char *d,const char *n){size_t z=strlen(d)+strlen(n)+1;char *p=malloc(z);snprintf(p,z,"%s%s",d,n);return p;}
unsigned long File_length(const char *path){FILE *f=fopen(path,"rb");long n;if(!f)return 0;fseek(f,0,SEEK_END);n=ftell(f);fclose(f);return n;}
void GFX2_Log(GFX2_Log_priority_T level,const char *fmt,...){va_list ap;va_start(ap,fmt);vprintf(fmt,ap);va_end(ap);}
int GFX2_MessageBox(const char *text,const char *caption,unsigned int type){printf("%s: %s\n",caption,text);return 1;}
int main(void)
{
 T_IO_Context c={0};int failed=0;const byte expected[12]={255,0,0,0,255,0,0,255,0,255,0,0};
 setvbuf(stdout,NULL,_IONBF,0);c.Width=2;c.Height=2;c.File_directory="SYS:";c.File_name="grafx-dt-roundtrip.png";c.Palette[0].R=255;c.Palette[1].G=255;
 Save_AmigaPNG(&c);printf("PNG save: %s\n",File_error?"FAIL":"PASS");
 if(File_error)failed=1;
 else {memset(result,0,sizeof(result));Load_AmigaDataType(&c);printf("PNG reload: %s\n",!File_error&&!memcmp(result,expected,12)?"PASS":"FAIL");if(File_error||memcmp(result,expected,12))failed=1;}
 GFX2_AmigaDataTypesClose();return failed?20:0;
}
