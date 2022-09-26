#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
char *fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)]='\0';
  return buf;
}

void find(char *path, char *name){
    char buf[512], *p;
    int fd;
    struct dirent de;//文件和其他文件的联系，
    struct stat st;//文件信息包括文件名字，大小,type属性用来判断是目录还是单个文件

    //按路径打开文件（广义文件包括目录和文件）
    if((fd = open(path, 0)) < 0){//如果文件路径是错的，就返回-1
    fprintf(2, "find: cannot open %s error\n", path);
    return;
  }
  //按打开文件的文件描述符记录文件的相关信息，通过文件结构体st记录
  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE://文件
    if(strcmp(fmtname(path),name)==0) printf("%s\n", path);//如果名字是对的，就可以输出路径了
    break;

  case T_DIR://目录
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){//这里判断了一下文件路径名超过了缓冲区预先设计好的最大长度
      printf("ls: path too long\n");
      break;
    }

    //目录加'/'编辑路径信息到p
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';

    //循环递归直到文件
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
     //如果连接文件inum=0的话说明是空目录，就不用继续递归了
      if(de.inum == 0)continue;
      if(de.name[0] == '.' && de.name[1] == 0) continue;
      if(de.name[0] == '.' && de.name[1] == '.' && de.name[2] == 0) continue;
    //如果不是的话，赋值当前的目录路径到指针p下
      memmove(p, de.name, strlen(de.name));
      p[strlen(de.name)] = '\0';
      //防止.和..循环递归
      find(buf,name);
      
    }
    break;
  }
  close(fd);//记得关闭每个一开始打开的文件标识符

}


int main(int argc ,char *argv[]){

   if(argc < 3){
        printf("Please input the right arg!\n");
        exit(0);
    }

    find(argv[1],argv[2]);//find 第一个参数是路径，第二个参数是文件名
    exit(0);

}