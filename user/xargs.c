#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
//查找param.h发现能够保存的最大的参数个数为32
int main(int argc ,char *argv[]){
   
    if(argc < 2){
        fprintf(2, "usage: xargs your_command\n");
        exit(1);
    }

    if (argc + 1 > MAXARG) {
      fprintf(2, "too many args\n");
      exit(1);
    }

    char *args[32];
    int argv_c=0;

    //首先要先保存exec自己的参数   
    for(int i=1; i<argc; i++){
        args[argv_c++] = argv[i];
        // printf(argv[i]);
        // printf("\n");
    }
    args[argc] = 0;
    // printf("argc:%d\n",argc);
    // printf("argv_c:%d\n",argv_c);
    char buf[32];
    char *pointer = buf;
    //读取
    while(read(0,pointer,1)>0){
        // printf("%d\n",*pointer);
        if(*pointer == '\n'){//检测到换行表示参数接受停止
            // printf("已经接收到参数");
            *pointer = 0;//结尾参数赋0
            if(fork()==0){//开始准备执行对应的程序
                //子线程执行换行后
                args[argv_c] = buf;
                // int j=0;
                // do{
                //     printf("args[%d]:%s\n",j,args[j]);
                //     j++;
                // }while (args[j]!=0);
                exec(args[0],args);
                exit(0);
            }
            else{
                wait(0);
            }
             pointer = buf;
            
        }
        else{
            ++pointer;
        }
    }

    exit(0);

}