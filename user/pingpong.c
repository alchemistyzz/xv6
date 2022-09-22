#include "kernel/types.h"
#include "user/user.h"
#define MASSAGESIZE 32
int main()
{
   int p[2];
   char buf[MASSAGESIZE];
   pipe(p);
   int pid = fork();
   if(pid==0){
        read(p[0],buf,MASSAGESIZE);
        printf("%d: received %s\n",getpid(),buf);
        write(p[1],"pong",MASSAGESIZE);
        
   }
   else if(pid>0){
        write(p[1],"ping",MASSAGESIZE);
        wait(0);
        read(p[0],buf,MASSAGESIZE);
        printf("%d: received %s\n",getpid(),buf);
   }
    exit(0); //确保进程退出
}
