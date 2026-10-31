/* GPL-2.0-only: run the actual GrafX2 bridge with installed system DataTypes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "loadsave.h"
#include "io.h"
#include "gfx2log.h"
#include "amigaopenup.h"
T_Document Main; byte Fore_color=3,Back_color=7;
extern byte *GFX2_OpenFontRender(const char *,const char *,int,int,int,int,int *,int *,T_Palette);
byte Best_color_perceptual_except(byte r,byte g,byte b,byte e){return e^1;}
signed char File_error;
static byte result[640*480*3];
void Load_AmigaDataType(T_IO_Context *);
void Save_AmigaPNG(T_IO_Context *);
void GFX2_AmigaDataTypesClose(void);
void Pre_load(T_IO_Context *c,short w,short h,long size,int format,enum PIXEL_RATIO ratio,byte bpp)
{if((w!=2||h!=2)&&(w!=640||h!=480)){File_error=1;return;}c->Width=w;c->Height=h;}
void Set_pixel(T_IO_Context *c,short x,short y,byte i)
{T_Components p=c->Palette[i];int n=(y*c->Width+x)*3;result[n]=p.R;result[n+1]=p.G;result[n+2]=p.B;}
void Set_pixel_24b(T_IO_Context *c,short x,short y,byte r,byte g,byte b)
{int n=(y*c->Width+x)*3;result[n]=r;result[n+1]=g;result[n+2]=b;}
byte Get_pixel(T_IO_Context *c,short x,short y){return (x+y)&1;}
char *Filepath_append_to_dir(const char *d,const char *n){size_t z=strlen(d)+strlen(n)+1;char *p=malloc(z);snprintf(p,z,"%s%s",d,n);return p;}
unsigned long File_length(const char *path){FILE *f=fopen(path,"rb");long n;if(!f)return 0;fseek(f,0,SEEK_END);n=ftell(f);fclose(f);return n;}
void GFX2_Log(GFX2_Log_priority_T level,const char *fmt,...){va_list ap;va_start(ap,fmt);vprintf(fmt,ap);va_end(ap);}
int GFX2_MessageBox(const char *text,const char *caption,unsigned int type){printf("%s: %s\n",caption,text);return 1;}
int datatype_roundtrip(void)
{
 T_IO_Context c={0};int failed=0;const byte expected[12]={255,0,0,0,255,0,0,255,0,255,0,0};
 static byte pixels[4]={0,1,1,0};
 c.Target_address=pixels;c.Pitch=2;setvbuf(stdout,NULL,_IONBF,0);c.Width=2;c.Height=2;c.File_directory="SYS:";c.File_name="grafx-dt-roundtrip.png";c.Palette[0].R=255;c.Palette[1].G=255;
 Save_AmigaPNG(&c);printf("PNG save: %s\n",File_error?"FAIL":"PASS");
 if(File_error)failed=1;
 else {memset(result,0,sizeof(result));Load_AmigaDataType(&c);printf("PNG reload: %s\n",!File_error&&!memcmp(result,expected,12)?"PASS":"FAIL");if(File_error||memcmp(result,expected,12))failed=1;}
 c.File_name="grafx-dt-alpha.png";c.Background_transparent=1;c.Transparent_color=1;Save_AmigaPNG(&c);printf("PNG binary-alpha export: %s\n",File_error?"FAIL":"PASS");if(File_error)failed=1;
 c.File_directory="SYS:GrafX2/";c.File_name="colour-fixture.png";memset(result,0,sizeof(result));Load_AmigaDataType(&c);printf("Full editor PNG load: %s %dx%d\n",File_error?"FAIL":"PASS",c.Width,c.Height);if(File_error)failed=1;
 {static const byte ref[16][3]={{255,255,255},{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,0},{0,255,255},{255,0,255},{255,128,0},{128,0,255},{0,128,128},{128,64,0},{170,109,93},{255,207,162},{96,96,96},{192,192,192}};
 unsigned mismatches=0;int x,y;
 for(y=0;y<480;y++)for(x=0;x<640;x++)
   if(memcmp(result+(y*640+x)*3,ref[(x/80+(y/60)*3)%16],3))mismatches++;
 printf("Full RGB colour fixture: %u mismatches\n",mismatches);
 if(mismatches)failed=1;}
 GFX2_AmigaDataTypesClose();return failed?20:0;
}

int main(void) {
 int failures=0,w=0,h=0;unsigned long len;unsigned i;byte *brush;T_Palette pal;
 static const byte compressed[]={120,156,99,96,100,98,102,97,101,99,231,224,228,226,230,225,229,227,23,16,20,18,22,17,21,19,151,144,148,146,150,145,149,147,87,80,84,82,86,81,85,83,215,208,212,210,214,209,213,211,55,48,52,50,54,49,53,51,183,176,180,178,182,177,181,179,119,112,116,114,118,113,117,115,247,240,244,242,246,241,245,243,15,8,12,10,14,9,13,11,143,136,140,138,142,137,141,139,79,72,76,74,78,73,77,75,207,200,204,202,206,201,205,203,47,40,44,42,46,41,45,43,175,168,172,170,174,169,173,171,111,104,108,106,110,105,109,107,239,232,236,234,238,233,237,235,159,48,113,210,228,41,83,167,77,159,49,115,214,236,57,115,231,205,95,176,112,209,226,37,75,151,45,95,177,114,213,234,53,107,215,173,223,176,113,211,230,45,91,183,109,223,177,115,215,238,61,123,247,237,63,112,240,208,225,35,71,143,29,63,113,242,212,233,51,103,207,157,191,112,241,210,229,43,87,175,93,191,113,243,214,237,59,119,239,221,127,240,240,209,227,39,79,159,61,127,241,242,213,235,55,111,223,189,255,240,241,211,231,47,95,191,125,255,241,243,215,239,63,127,255,253,103,24,245,255,168,255,71,253,63,234,255,81,255,143,250,127,212,255,163,254,31,245,255,168,255,71,253,63,234,255,81,255,143,250,127,4,248,31,0,96,174,248,106};byte unpacked[4096];
 byte *input=malloc(640*32),*output=malloc(640*32*4),table[1024];
 setvbuf(stdout,NULL,_IONBF,0);GFX2_OpenUpInit();
 for(i=0;i<256;i++){table[i*4]=i==7?0:255;table[i*4+1]=i;table[i*4+2]=255-i;table[i*4+3]=i^85;}
 for(i=0;i<640*32;i++)input[i]=i;
 GFX2_OpenUpExpand(input,640,output,2560,640,32,table);
 for(i=0;i<640*32;i++)if(memcmp(output+i*4,table+input[i]*4,4)){failures++;break;}
 printf("OpenMulticore/serial conversion: %s\n",i==640*32?"PASS":"FAIL");free(input);free(output);
 len=sizeof(unpacked);i=GFX2_OpenZUncompress(unpacked,&len,compressed,sizeof(compressed));
 if(i || len!=4096)failures++;else for(i=0;i<4096;i++)if(unpacked[i]!=(byte)i){failures++;break;}
 printf("OpenZlib decompression: %s (%lu bytes)\n",i==4096?"PASS":"FAIL",len);
 len=4;if(GFX2_OpenZUncompress(unpacked,&len,compressed,sizeof(compressed))!=-5)failures++;
 len=4096;if(GFX2_OpenZUncompress(unpacked,&len,compressed,sizeof(compressed)-5)==0)failures++;
 printf("OpenZlib buffer/truncation checks: completed\n");
 memset(&Main,0,sizeof(Main));Main.palette[3].R=255;Main.palette[7].B=255;
 brush=GFX2_OpenFontRender("Ag caf\351\nTest","SYS:DejaVuSans.ttf",24,1,1,1,&w,&h,pal);
 if(!brush || w<20 || h<24)failures++;
 printf("OpenFont AA/bold/italic multiline brush: %s %dx%d\n",brush?"PASS":"FAIL",w,h);
 if(brush){FILE *f=fopen("SYS:fontbrush.pgm","wb");fprintf(f,"P5\n%d %d\n255\n",w,h);for(i=0;i<(unsigned)(w*h);i++){byte a=brush[i];if(a==7)a=0;else if(a==0)a=7;fwrite(&a,1,1,f);}fclose(f);free(brush);}
 brush=GFX2_OpenFontRender("Test","SYS:DejaVuSans.ttf",18,0,0,0,&w,&h,pal);
 if(!brush)failures++;else {for(i=0;i<(unsigned)(w*h);i++)if(brush[i]!=3&&brush[i]!=7){failures++;break;}free(brush);}
 printf("OpenFont solid palette: completed\n");
 failures+=datatype_roundtrip()!=0;GFX2_OpenUpClose();printf("OPENUP TARGET RESULT: %s (%d failures)\n",failures?"FAIL":"PASS",failures);
 return failures?20:0;
}
