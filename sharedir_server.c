#include "stdio.h"
#include "stdlib.h"
#include "netinet/in.h"
#include "sys/socket.h"
#include "strings.h"
#include "string.h"
#include "sys/types.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "mysql/mysql.h"
#include "signal.h"
#include "sys/stat.h"
#include "sys/file.h" 
#include <dirent.h>

#define BACKLOG 5
#define BUFSIZE 1024*1024*5
#define EXSITED 0
#define NOTEXSITED 1
#define SUCCESS 1
#define UNSUCCESS -1
#define PATH "/home/chen/code/sockets/tmp"
#define SVRPORT 30001

void handler()
{
  printf("handler\n");
  alarm(0);
}

MYSQL init_mysql(MYSQL mysql)
{
  if(mysql_init(&mysql)==NULL)
  {
    printf("init error\n");
    exit(1);
  }
  if(mysql_real_connect(&mysql,
            "localhost",
            "root",
            "jkl;'",
            "ACCOUNT",
            0,
            NULL,
            0)==NULL)
  {
    printf("mysql connect error\n");
    exit(1);
  }
  return mysql;
}


int check(char account[],char password[])
{// if correct return 1,else return 0;
  MYSQL mysql;
  MYSQL_RES* respond;
  MYSQL_ROW row;
  char sql_select[100];
  bzero(sql_select,strlen(sql_select));
  mysql=init_mysql(mysql);
  sprintf(sql_select,"select useraccount,password from user where useraccount='%s' and password='%s'",account,password);
  if(mysql_query(&mysql,sql_select)!=0)
  {
    printf("query error\n");
    printf("%s\n",mysql_error(&mysql) );
    mysql_close(&mysql);
    exit(1);
  }
  respond=mysql_store_result(&mysql);
  if(mysql_num_rows(respond)==1) {mysql_close(&mysql);return 1;}
  else  {mysql_close(&mysql);return -1;}
}


int isexist(char account[],char table[])
{   //if exist return 1 else return 0
  MYSQL_RES *respond;
  MYSQL_ROW row;
  MYSQL mysql;
  char sql_select[100];

  mysql=init_mysql(mysql);
  sprintf(sql_select,"select * from %s where useraccount='%s'",table,account);
  if(mysql_query(&mysql,sql_select)!=0)
  {
    printf("query error\n");
    printf("%s\n",mysql_error(&mysql) );
    mysql_close(&mysql);
    exit(1);
  }
  respond=mysql_store_result(&mysql);
  if(mysql_num_rows(respond)==1) 
    {mysql_close(&mysql);return EXSITED;}
  else {mysql_close(&mysql);return NOTEXSITED;}
} 

int signupsql(char newaccount[],char newpassword[])
{
  MYSQL mysql;
  char sql_insert[200],sql_create[200];
  mysql=init_mysql(mysql);

  if(isexist(newaccount,"user")==EXSITED)
  {
    printf("the account already exsits,please change another\n");
    mysql_close(&mysql);
    return EXSITED;
  }

  sprintf(sql_insert,"insert into user values('%s','%s')",newaccount,newpassword);
  if(mysql_query(&mysql,sql_insert)!=0)  
  {
    printf("query error\n");
    printf("%s\n",mysql_error(&mysql) );
    mysql_close(&mysql);
    return UNSUCCESS;
  }
  mysql_close(&mysql);
  printf("sign up successful!\n");
  return SUCCESS;
}
void signup(int newfd)
{
  char newaccount[20],newpassword[20],writebuf[BUFSIZE];
  int nbytes=0,end;

  bzero(newaccount,sizeof(newaccount));
  nbytes=read(newfd,newaccount,sizeof(newaccount));
  newaccount[nbytes]='\0';
  printf("received useraccount: %s\n",newaccount);

  bzero(newpassword,sizeof(newpassword));
  nbytes=read(newfd,newpassword,sizeof(newpassword));
  newpassword[nbytes]='\0';
  printf("received password: %s\n",newpassword);
  
  end=signupsql(newaccount,newpassword);

  if(end==EXSITED)
  {
    sprintf(writebuf,"0");
    write(newfd,writebuf,strlen(writebuf));
    return;
  }
  else if(end==UNSUCCESS)
  {
    sprintf(writebuf,"-1");
    write(newfd,writebuf,strlen(writebuf));
    return;
  }
  else
  {
    sprintf(writebuf,"1");
    write(newfd,writebuf,strlen(writebuf));
    char path[100]="/home/chen/code/sockets/tmp/";
    strcat(path,newaccount);
    if(mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)<0)
    {
      printf("create dir error\n");
      exit(1);
    }
    return;
  }
}

void login(int newfd)
{
  char account[20],password[20],writebuf[BUFSIZE];
  int nbytes=0,end;
  bzero(account,sizeof(account));
  nbytes=read(newfd,account,sizeof(account));
  account[nbytes]='\0';
  printf("received  client : %s\n",account);

  bzero(password,sizeof(password));
  nbytes=read(newfd,password,sizeof(password));
  password[nbytes]='\0';

  if(check(account,password)<0)
  {
    printf("your account or password is incorrect,please input again:\n");
    sprintf(writebuf,"-1");
    write(newfd,writebuf,strlen(writebuf));
    return ;
  }
  else 
  {
    printf("login successful!\n");
    sprintf(writebuf,"1");
    write(newfd,writebuf,strlen(writebuf));
    return ;
  }
}

void writefile(int newfd,FILE *fp)
{
  char *buf=(char*)malloc(sizeof(buf)*BUFSIZE);
  int length=0;
  bzero(buf,BUFSIZE);

  read(newfd,buf,sizeof(buf));
  long int filesize=atoi(buf);
  struct sigaction sa;
  sa.sa_handler=handler;
  sa.sa_flags=SA_INTERRUPT;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGALRM,&sa,NULL);
  bzero(buf,BUFSIZE);
  alarm(20);
  long int recvsize=0;
  while(recvsize<filesize)
  {
    length = read(newfd,buf,BUFSIZE);
    if(length<0)
    {
       printf("Recieve Data From Client Failed!\n");
       break;
    }
    if(length==0)
    {
      printf("receive ok\n"); 
      break;
    }
    alarm(0);
    printf("%d\n",length);
    recvsize=recvsize+length;
    int write_length=fwrite(buf,sizeof(char),length,fp);
    printf("%dKB\n",write_length/1024);
    if(write_length<length)
    {
      printf("File: Write Failed!\n");
      break;  
    }
    alarm(20);
    bzero(buf,BUFSIZE);
  }
  free(buf);
}


void readfile(int newfd,FILE *fp,char*path)
{
  char *buf=(char*)malloc(sizeof(buf)*BUFSIZE);
  int length=0;
  struct stat st;
  stat(path,&st);
  bzero(buf,BUFSIZE);
  sprintf(buf,"%ld",st.st_size);
  sleep(1);
  write(newfd,buf,strlen(buf));
  sleep(1);
  bzero(buf,BUFSIZE);
  while((length=fread(buf,sizeof(char),BUFSIZE,fp))>0)
  {
    if(length<0)
    {
       printf("write data to client error\n");
       break;
    }

    length=send(newfd,buf,length,0);
    printf("length--%d\n",length);
    bzero(buf,BUFSIZE);
  }
  sleep(1);
  flock(fp->_fileno,LOCK_UN);
  fclose(fp);
  free(buf);
  return;
}
void upload(int newfd)
{
  char writebuf[20],filename[100],readbuf[100];
  bzero(filename,sizeof(filename));
  read(newfd,filename,sizeof(filename));
  char path[200]=PATH; 
  strcat(path,filename); 
  
  
  FILE *fp = fopen(path, "w"); 
  if(NULL == fp) 
  { 
    printf("File:\t%s Can Not Open To Write\n",path);
    bzero(writebuf,sizeof(writebuf));
    sprintf(writebuf,"-1");
    write(newfd,writebuf,strlen(writebuf)); 
    return ; 
  }
  int lockend=flock(fp->_fileno,LOCK_EX);
  bzero(writebuf,sizeof(writebuf));
  sprintf(writebuf,"1");
  write(newfd,writebuf,strlen(writebuf));
  printf("%d\n",lockend);

  writefile(newfd,fp); //remix

  flock(fp->_fileno,LOCK_UN);
  fclose(fp);
  return;

}


void lookup(char *s,int newfd)
{
    DIR *dir;
    char writebuf[200];
    struct dirent *ptr;
    struct stat file;
    strcat(s,"/");
    dir = opendir(s);
    while((ptr = readdir(dir)) != NULL)
    {
    if(ptr->d_name[0]=='.')
      continue;
    char path[200]="\0";
    strcpy(path,s);
    strcat(path,ptr->d_name);
    stat(path,&file);
    if(S_ISDIR(file.st_mode))
      {
        lookup(path,newfd);
      }
      else
      {
        bzero(writebuf,sizeof(writebuf));
        strcpy(writebuf,path);
        //sprintf(writebuf,path);
        send(newfd,writebuf,strlen(writebuf),0);
        sleep(1);
      }
    }
    closedir(dir);
    return ;
}

void download(int newfd)
{
  char filename[100],writebuf[100];
  bzero(filename,sizeof(filename));
  read(newfd,filename,sizeof(filename));
  char path[200]=PATH;
  strcat(path,"/");
  strcat(path,filename);
  printf("%s\n",path);
  FILE *fp = fopen(path, "r"); 
  if(NULL == fp) 
  { 
    printf("File:\t%s Can Not Open To read\n",path);
    bzero(writebuf,sizeof(writebuf));
    sprintf(writebuf,"-1");
    write(newfd,writebuf,strlen(writebuf)); 
    return; 
  }
  int lockend=flock(fp->_fileno,LOCK_SH);
  int length=0;
  readfile(newfd,fp,path);
  return;
}


void share(int newfd)
{
  char filename[100],writebuf[100],readbuf[100],account[20]="\0",path[100]="\0",basepath[100]="\0";
 
  read(newfd,account,sizeof(account));
  
  read(newfd,basepath,sizeof(basepath)); //wrong check
  struct stat filetime;
  int baselen=strlen(basepath);
  while(1)
  {
    bzero(&filetime,sizeof(filetime));
    bzero(path,sizeof(path));
    bzero(filename,sizeof(readbuf));    
    bzero(readbuf,sizeof(readbuf));
    read(newfd,filename,sizeof(filename)); //wrong check
    if(filename[0]=='0') break;
    strcat(path,PATH);
    strcat(path,"/");
    strcat(path,account);
    char newpath[100]="\0";
    int count=0;
    for(int i=baselen-1,j=0;i<strlen(filename);i++,j++)
    {
      newpath[j]=filename[i];
    }
    printf("%s\n",newpath);
    strcat(path,newpath);
    printf("path--%s\n",path);
    char dirpath[100];
    strcpy(dirpath,path);
    for(int i=strlen(path)-1;i>=0;i--)
    {
      if(dirpath[i]!='/') dirpath[i]='\0';
      else break;
    }
    mkdir(dirpath,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    stat(path,&filetime);
    long int lastchange=filetime.st_mtime;
    
    bzero(readbuf,sizeof(readbuf));
    read(newfd,readbuf,sizeof(readbuf));
    long int cmptime=atoi(readbuf);

    if(cmptime<lastchange)
    {
      bzero(writebuf,sizeof(writebuf));
      sprintf(writebuf,"-1");
      write(newfd,writebuf,strlen(writebuf));
      continue;
    }

    bzero(writebuf,sizeof(writebuf));
    sprintf(writebuf,"1");
    write(newfd,writebuf,strlen(writebuf));
    
    FILE *fp=fopen(path,"w");
    if(fp==NULL) printf("fopen error\n");
    writefile(newfd,fp);
    fclose(fp);
  }
  return;
}

void isdir(char *path,int newfd)
{
  char account[20],basepath[100]=PATH,writebuf[100],filename[100],readbuf[100];
  struct dirent *ptr;
  struct stat file;
  DIR *dir = opendir(path);
  strcat(path,"/");
  while((ptr =  readdir(dir)) != NULL)
  {
    char tmppath[100];
    strcpy(tmppath,path);
  if(ptr->d_name[0]=='.')
    continue;
  strcat(tmppath,ptr->d_name);
    stat(tmppath,&file);
  if(S_ISDIR(file.st_mode))
    {
      isdir(tmppath,newfd);
    }
    else
    {
      bzero(writebuf,sizeof(writebuf));
      sprintf(writebuf,ptr->d_name);
      write(newfd,writebuf,strlen(writebuf));
      sleep(1);
      bzero(writebuf,sizeof(writebuf));
      long int filetime=file.st_mtime;
      sprintf(writebuf,"%ld",filetime);
      write(newfd,writebuf,strlen(writebuf));   
      bzero(readbuf,sizeof(readbuf));
      sleep(1); 
      if(read(newfd,readbuf,sizeof(readbuf))<=0) break;
      if(readbuf[0]=='1')
      {
        FILE *fp=fopen(tmppath,"r");
        if(fp==NULL) printf("fopen error\n");
        readfile(newfd,fp,tmppath);
        sleep(1);
      }
    }
  }
  closedir(dir);
  sleep(1);
}
void update(int newfd)
{
  char account[20],basepath[100]=PATH;
  bzero(account,sizeof(account));
  read(newfd,account,sizeof(account));

  strcat(basepath,"/");
  strcat(basepath,account);
  isdir(basepath,newfd);
}
int main()
{
  int sockfd=socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd==-1)
  {
    printf("socket creation error\n"); 
   exit(1);
  }
  int on=1;
  setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int));

  struct sockaddr_in srvaddr;
  bzero(&srvaddr,sizeof(srvaddr));
  srvaddr.sin_family=AF_INET;
  srvaddr.sin_port=htons(SVRPORT);
  srvaddr.sin_addr.s_addr=htonl(INADDR_ANY);
 
  if(bind(sockfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr))==-1)
  {
    printf("bind error\n");
    close(sockfd);
    exit(1);
  }

  if(listen(sockfd, BACKLOG)==-1)
  {
    printf("listen error\n");
    close(sockfd);
    exit(1);
  }

  struct sockaddr_in clientaddr;
  unsigned int addr_size;
  bzero(&clientaddr,sizeof(clientaddr));
  addr_size=sizeof(struct sockaddr_in);
  while(1)
  {
  int newfd=accept(sockfd,(struct sockaddr *)&clientaddr, &addr_size);
  if(newfd==-1)
  {
    printf("accept error\n");
    continue;
  }
  printf("received from client: %s, %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port)); 
  
  int pid=fork();
  if(pid<0)
  {
    printf("fork error \n");
    continue;
  }
  else if(pid==0)
  {//son
    close(sockfd);
    char account[20];
    char *buf=(char *)malloc(sizeof(char)*BUFSIZE);
    struct sigaction sa;
    sa.sa_handler=handler;   
    sa.sa_flags=SA_INTERRUPT;      
    sigemptyset(&sa.sa_mask);            
    sigaction(SIGALRM,&sa,NULL);
    while(1)
    {
      bzero(buf,BUFSIZE);
      alarm(200);
      int nbytes=read(newfd,buf,sizeof(buf));
      if(nbytes==0)
      {
        printf("client interrupt \n");
        break;
      }
      if(nbytes<0) //{printf("time out \n"); break;}
      {
        printf("time out\n");
        bzero(buf,sizeof(buf));
        sprintf(buf,"3");
        write(newfd,buf,strlen(buf));
        break;
      }
      printf("received choose:%s\n",buf);
      alarm(0);
      char s[200]="\0",writebuf[20];
      strcpy(s,PATH);
      switch(buf[0])
      {

        case '0': signup(newfd);break;
        case '1': login(newfd);break;
        case '2': close(newfd);exit(0);
        case '3': 
        { bzero(writebuf,sizeof(writebuf));
          sprintf(writebuf,PATH);
          write(newfd,writebuf,strlen(writebuf));
          lookup(s,newfd);
          bzero(writebuf,sizeof(writebuf));
          sprintf(writebuf,"3");
          write(newfd,writebuf,strlen(writebuf));
          break;
        }
        case '4': upload(newfd);break;
        case '5': download(newfd);break;
        case '6': share(newfd);break;
        case '7': 
        { update(newfd);
          bzero(writebuf,sizeof(writebuf));
          sprintf(writebuf,"0");
          write(newfd,writebuf,strlen(writebuf));
          break;
        }
        default :break;
      }
    }
      close(newfd);
      exit(0);
    }
  else
  {//father
    close(newfd);
  } 
}
  return 0;
}

