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
#include "dirent.h"
#include "time.h"

#define ATM_NUM 5
#define BACKLOG	
#define BUFSIZE 1024
#define EXSITED 0
#define NOTEXSITED 1 
#define SUCCESS 1
#define UNSUCCESS -1
#define SVRPORT 30001
char ATM_IP[ATM_NUM][32]={
	"",
	"",
	"",
	"",
	"",
};
struct Head
{
	char account[19];
	char password[6];
	char username[32];
	char servertype;//0:取款 1：存款 2：转账 3：打印
	char result;
	unsigned int money;
	char inaccount[19];
	char atmaddress[32];
	struct tm time;
};
MYSQL mysql;




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
			"lll123!",
			"ATMserver",
			0,
			NULL,
			0)==NULL)
	{
		printf("MySQL connect error!\n");
		exit(1);
	}
	return mysql;
}

/*
	create table user(
		account varchar(20) primary key,
		password varchar(6) not NULL,
		name varchar(32) not NULL,
		money int
	);
	insert into user values('Sttide','123456','wang','1000');
*/

void mybzero(char goalstrings[])
{
	bzero(goalstrings,sizeof(goalstrings));
}

int check(char account[],char password[])
{	//prove log in
	MYSQL_RES* respond;
	MYSQL_ROW row;
	char sql_select[100];
	mybzero(sql_select);
	
	sprintf(sql_select,"select * from user where account='%s' and password='%s'",account,password);
	if(mysql_query(&mysql,sql_select))
	{
		printf("Query Error!\n");
		printf("%s\n",mysql_error);
		mysql_close(&mysql);
		exit(1);
	}
	respond=mysql_store_result(&mysql);
  	if(mysql_num_rows(respond)==1) 
  	{
  		while(row = mysql_fetch_row(respond))  
    	{  
        	printf("%s %s %s %s\n",row[0],row[1],row[2],row[3]);  
    	} 
  		mysql_free_result(respond);
  		return 1;
  	}
  	else
  	{ 
		mysql_free_result(respond);
  		return -1;
  	}
}

int isexist(char account[])
{   //if exist return 1 else return 0
  MYSQL_RES *respond;
  MYSQL_ROW row;
  char sql_select[100];

  mybzero(sql_select);
  sprintf(sql_select,"select * from user where account='%s'",account);
  if(mysql_query(&mysql,sql_select)!=0)
  {
    printf("query error\n");
    printf("%s\n",mysql_error(&mysql) );
 	mysql_free_result(respond);
    exit(1);
  }
  respond=mysql_store_result(&mysql);
  if(mysql_num_rows(respond)==1) 
    {mysql_free_result(respond);return EXSITED;}
  else {mysql_free_result(respond);return NOTEXSITED;}
} 

void handler()
{
	//signal chuli hanshu
	printf("handler\n");
	alarm(0);
}

void savings()
{
	return;
}

void withdraws()
{
	return;
}

void transfer()
{
	return;
}

void pirntlist()
{
	return;
}

int main()
{
	mysql=init_mysql(mysql);
	check("Sttide","123456");
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		printf("Socket is created error!\n");
		exit(1);
	}
	int on=1;
	setsockop(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int));

	struct  sockaddr_in srvaddr;
	bzero(&srvaddr,sizeof(srvaddr));
	srvaddr.sin_family=AF_INET;
	srvaddr.sin_port=htons(SVRPORT);
	srvaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	if(bind(sockfd,(struct  sockaddr*)&srvaddr,sizeof(srvaddr))==-1)
	{
		printf("Bind error!\n");
		close(sockfd);
		exit(1);	
	}

	if(listen(sockfd,BACKLOG)==-1)
	{
		printf("Listen error!\n");
		close(sockfd);
		exit(1);
	}

	struct sockaddr_in clientaddr;
	unsigned int addr_size;
	bzero(&clientaddr,sizeof(clientaddr));
	addr_size = sizeof(clientaddr);
	while(1)
	{
		int newfd=accept(sockfd,(struct sockaddr *)&clientaddr,&addr_size);
		if(newfd==-1)
		{
			printf("Accept error!\n");
   			continue;
  		}
  		printf("Received from client: %s, %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port)); 
 
		int pid=fork();
  		if(pid<0)
 		{
    		printf("Fork error!\n");
    		continue;
  		}
  		else if(pid==0)
		{
			close(sockfd);
			struct Head usermsg;
			struct sigaction sa;
			sa.sa_handler=handler;
			sa.sa_flags=SA_INTERRUPT;
			sigemptyset(&sa.sa_mask);
			sigaction(SIGALRM,&sa,NULL);
			char buf[1024];
			while(1)
			{
				int nbytes=read(newfd,buf,sizeof(buf));
				if(nbytes==0)
				{
					printf("Client interrupt!\n");
					break;
				}
				if(nbytes<0) //{printf("time out \n"); break;}
				{
					printf("Time out!\n");
					mybzero(buf);
					sprintf(buf,"3"); ///????
					write(newfd,buf,strlen(buf));
					break;
				}
				printf("Received package:%s\n", buf);
				alarm(0);
				usermsg=(struct Head*)buf;


			}
		}
	}
	mysql_close(&mysql);
	return 0;
}