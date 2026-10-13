#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
int main(void){
 const char *paths[]={"System:Open2Ports/OpenJazz/","System:Open2Ports/OpenJazz","PROGDIR:","ENVARC:"};unsigned i;struct stat st;int x,err;
 for(i=0;i<4;i++){errno=0;x=access(paths[i],W_OK);err=errno;printf("ACCESS %s rc=%d errno=%d\n",paths[i],x,err);errno=0;x=stat(paths[i],&st);err=errno;printf("STAT %s rc=%d errno=%d mode=%lo\n",paths[i],x,err,x==0?(unsigned long)st.st_mode:0UL);}
 errno=0;x=open("System:Open2Ports/OpenJazz/.__open2_probe_unique_20261013",O_WRONLY|O_CREAT|O_EXCL,0600);err=errno;
 printf("CREATE_EXCLUSIVE rc=%d errno=%d\n",x,err);
 if(x>=0){write(x,"probe\n",6);close(x);printf("DELETE rc=%d\n",unlink("System:Open2Ports/OpenJazz/.__open2_probe_unique_20261013"));}
 return 0;
}
