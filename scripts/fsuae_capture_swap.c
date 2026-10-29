/* SPDX-License-Identifier: MIT
 * Read the actual current GL framebuffer before the real SDL swap.
 * Does not replace rendering, CPU execution, ROM or application code.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef void (*Swap)(void*);
typedef void* (*Proc)(const char*);
void SDL_GL_SwapWindow(void* window){
 static Swap real_swap;
 static Proc get_proc;
 static unsigned frames;
 static unsigned every;
 static int configured;
 if(!configured){const char* value=getenv("FS_CAPTURE_EVERY");if(value)every=(unsigned)strtoul(value,NULL,10);configured=1;}
 if(!real_swap)real_swap=(Swap)dlsym(RTLD_NEXT,"SDL_GL_SwapWindow");
 if(!get_proc)get_proc=(Proc)dlsym(RTLD_NEXT,"SDL_GL_GetProcAddress");
 ++frames;
 if(get_proc && (frames==150 || frames==500 || frames==900 || (every && frames%every==0))){
  void (*get_int)(unsigned,int*)=(void(*)(unsigned,int*))get_proc("glGetIntegerv");
  void (*read_pixels)(int,int,int,int,unsigned,unsigned,void*)=(void(*)(int,int,int,int,unsigned,unsigned,void*))get_proc("glReadPixels");
  void (*pixel_store)(unsigned,int)=(void(*)(unsigned,int))get_proc("glPixelStorei");
  int viewport[4]={0},alignment=4;
  if(get_int && read_pixels && pixel_store){
   get_int(0x0ba2,viewport);get_int(0x0d05,&alignment);
   int width=viewport[2],height=viewport[3];
   if(width>0 && height>0 && width<=4096 && height<=4096){
    size_t row=(size_t)width*3;unsigned char* p=malloc(row*(size_t)height);
    if(p){pixel_store(0x0d05,1);read_pixels(0,0,width,height,0x1907,0x1401,p);pixel_store(0x0d05,alignment);
     char name[1024];const char* dest=getenv("FS_CAPTURE_DIRECTORY");if(!dest)dest=".";
     snprintf(name,sizeof name,"%s/fsuae-frame-%u.ppm",dest,frames);FILE* f=fopen(name,"wb");
     if(f){fprintf(f,"P6\n%d %d\n255\n",width,height);for(int y=height-1;y>=0;--y)fwrite(p+(size_t)y*row,1,row,f);fclose(f);fprintf(stderr,"CAPTURE real OpenGL framebuffer frame=%u %dx%d path=%s\n",frames,width,height,name);}
     free(p);
    }
   }
  }
 }
 if(real_swap)real_swap(window);
}
