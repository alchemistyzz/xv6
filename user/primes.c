#include "kernel/types.h"
#include "user/user.h"
void prime_solution(int p1[],int p2[])
//p1是负责读取父进程质数集信息的管道
//p2是负责写入子进程质数集信息的管道
{
    int pid = fork();
    
    if(pid==0){//子线程
      int to_prime[1];	
      if(read(p1[0],to_prime,sizeof(int))!=sizeof(int))
      {
	      close(p1[0]);
          close(p2[0]);
          close(p2[1]);	  
	      exit(0);
      }
      int prime=to_prime[0];
     
      printf("prime %d\n",prime);
        while(read(p1[0],to_prime,sizeof(int))==sizeof(int)){

            if(to_prime[0]%prime!=0){
                write(p2[1],to_prime,sizeof(int));            
            }

        }
        close(p1[0]);
        close(p2[1]);
        pipe(p1);
        prime_solution(p2,p1);
        close(p1[1]);
        close(p2[0]);
        exit(0); 
    }
    else if(pid >0){
        close(p1[0]);
        close(p1[1]);
        close(p2[0]);
        close(p2[1]);
        wait(0);
        exit(0);
    }

}

 int main()
 {  //把main线程作为父线程开始递归调用prime_solution子线程
    int p1[2];
    int p2[2];
    pipe(p1);
    pipe(p2);
    int i=2;
    for(i=2; i<=35 ;i++){
        write(p1[1],&i,sizeof(int));//将初始候选质数集写入管道p1
    }
    close(p1[1]);
    prime_solution(p1,p2);
    close(p1[0]);
    close(p2[0]);
    close(p2[1]);
    wait(0);
    exit(0);
 }