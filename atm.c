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

#include <time.h>

#define PORT 30000
#define SRVIP "127.0.0.1"
#define MYADDRESS "xdu"
#define FILEDIR ""
struct PACKAGE
{
	char account[19];
	char password[6];
	char username[32];
	int servertype;// 0：登录 1:存款 2：取款 3：转账 4:查询 5：打印 6：退出
	int result;
	unsigned int money;
	char inaccount[19];
	char atmaddress[32];
	char time[32];
};
struct PACKAGE user;
int sockfd;
MYSQL mysql;

void  init_mysql()
{
  if(mysql_init(&mysql)==NULL)
  {
    printf("init error\n");
    exit(1);
  }
  if(mysql_real_connect(&mysql,
            "localhost",
            "root",
            "zc0322",
            "LOG",
            0,
            NULL,
            0)==NULL)
  {
    printf("mysql connect error\n");
    exit(1);
  }
  return ;
}

void  writelog(int flag)
{
  char sql_insert[200];
  sprintf(sql_insert,"insert into locallog values('%s','%s','%d','%d','%u','%s','%s','%s')",
  	user.account,user.username,user.servertype,flag,user.money,user.inaccount,user.atmaddress,user.time);
  if(mysql_query(&mysql,sql_insert)!=0)  
  {
  	error_process(501)
    return ;
  }
  return ;
}

void chagelog(int flag)
{
	char sql_update[200];
  	sprintf(sql_update,"update locallog set flag='%d' ",flag);
  if(mysql_query(&mysql,sql_update)!=0)  
  {
  	error_process(501)
    return ;
  }
  return ;
}

void checklog()
{
	MYSQL_RES* respond;
  	MYSQL_ROW row;
  	char sql_select[100];
 	bzero(sql_select,strlen(sql_select));
	sprintf(sql_select,"select * from locallog");
	if(mysql_query(&mysql,sql_select)!=0)
	{
	  error_process(501);
	  return;
	}
	respond=mysql_store_result(&mysql);
	num_fields=mysql_num_fields(respond);
	struct PACKAGE tmp;
	sigaction(SIGALRM,initsa(),NULL);
	while(mysql_fetch_row(respond))
	{
		int flag;
		strcpy(tmp.account,row[0]);
		strcpy(tmp.username,row[1]);
		tmp.servertype=row[2]+10;
		tmp.result=*row[3];
		flag=*row[3];
		tmp.money=*row[4];
		strcpy(tmp.inaccount,row[5]);
		strcpy(tmp.atmaddress,row[6]);
		strcpy(tmp.time,row[7]);
		write(sockfd,&tmp,sizeof(tmp));
		alarm(20);
		char buf[1024];
		bzero(buf,sizeof(buf));
		int nbytes=read(sockfd,buf,sizeof(buf));
		if(nbytes<0) {error_process(601);return;}
		tmp=(struct PACKAGE *)buf;
		if(flag==0&&tmp->result==-1)
		{//delete

		}
		else if(flag==0&&tmp->result==0)
		{//back

		}
		else if((flag==1&&tmp->result==0)||(flag==1&&tmp->result==1))
		{//delete both

		}

	}
}
void error_process(int errornum)
{
	switch(errornum)
	{
		case 1:printf("error001:ATM is broken\n");
				 exit(1);
		case 2:printf("error002:Internet error\n");
				 close(sockfd);
				 exit(1);
		case 101:printf("error101:savings broken\n");
				 return;
		case 102:printf("error102:connection timeout\n");	
				 printf("money out box ,please check it and try again\n");
		case 201:printf("error201:connection timeout\n");
		case 202:printf("error202:error exception\n");
		case 301:printf("error301:transfer timeout")
		case 302:printf("error302:unexistent account\n")
				 return;
		case 303:printf("error303:error exception\n");

		case 401:printf("error401:lookuo timeout\n");
		case 402:printf("error402:lookup error\n");
		case 501:printf("error501:database query error");
				 printf("%s\n",mysql_error(&mysql) );
				 return;
		case 601:printf("error601:checklog timeout\n");
		default:break;

	}
}
void handler()
{
	return;
}
struct sigaction * initsa()
{
	struct sigaction *sa=NULL;
    sa->sa_handler=handler;   
    sa->sa_flags=SA_INTERRUPT;      
    sigemptyset(&sa->sa_mask); 
    return sa;           
}



void savings()
{            
    sigaction(SIGALRM,initsa(),NULL);
	printf("please input your saving amount:\n");
	unsigned int amount;
	scanf("%u",&amount);
	user.money=amount;
	user.servertype=1;
	time_t timer=time(NULL);
	user.time=ctime(&timer);
	writelog(0);
	printf("please put your money in the box");
	printf("~~~~~~~~processing~~~~~~~~\n");
	write(sockfd,&user,sizeof(user));
	char buf[1024];
	bzero(buf,sizeof(buf));
	alarm(20);
	//waiting  first ,if recv error, return 
	int nbytes=read(sockfd,buf,sizeof(buf));
	printf("%d",bytes);
	if(nbytes<0)
	{
		error_process(102);
		return;
	}
	struct PACKAGE *tmp;
	tmp=(struct PACKAGE *)buf;
	if(tmp->result==1)
	{
		//box in bank
		chagelog(1);
		user.result=1;
		//fscanf()
		write(sockfd,&user,sizeof(user));
		printf("successfully savings\n");
		printf("whether choose to printlist:(yse is 1,no is 0)\n");
		int isprint;
		sacnf("%d",&isprint);
		if(isprint==1) printlist();
		return ;
	}
	else if(tmp->result==0)
 	{
 		error_process(101);
 		return;
	}
}

void withdraws()
{
	sigaction(SIGALRM,initsa(),NULL);
	int isenough=0;
	while(isenough==0)
	{
		unsigned int amount;
		printf("please input your amount:\n");
		scanf("%u",&amount);
		lookup();
		if(user.money>=amount)
		{
			user.servertype=2;
			time_t timer=time(NULL);
			user.time=ctime(&timer);
			writelog(0);
			write(sockfd,&user,sizeof(user));
			char buf[1024];
			bzero(buf,sizeof(buf));
			alarm(20); 
			int nbytes=read(sockfd,buf,sizeof(buf));
			if(nbytes<0) {error_process(201);return;}
			alarm(0);
			struct PACKAGE *tmp;
			tmp=(struct PACKAGE *)buf;
			if(tmp->result==1)
			{
				isenough=1;
				printf("Money is being spat. Please accept your bill\n");
				user.result=1;
				chagelog(1);
				write(sockfd,(char *)&user,sizeof(user));
				printf("whether choose to printlist:(yse is 1,no is 0)\n");
				int isprint;
				sacnf("%d",&isprint);
				if(isprint==1) printlist();
			}
			else
			{	
				error_process(202);
				return;
			}
		}
		else 
		{
			printf("your balance is not enough\n");
			continue;
		}

	}
	return;
}

void transfer()
{
	printf("please input your inaccount:\n");
	scanf("%s",user.inaccount);
	sigaction(SIGALRM,initsa(),NULL);

	int isenough=0;
	while(isenough==0)
	{
		unsigned int amount;
		printf("please input your  amount:\n");
		scanf("%u",&amount);
		user.money=amount;
		lookup();
		if(user.money>=amount)
		{
			isenough=1;
			user.servertype=3;
			write(sockfd,&user,sizeof(user));
			char buf[1024];
			bzero(buf,sizeof(buf));
			writelog(0);
			alarm(20);
			int nbytes=read(sockfd,buf,sizeof(buf));
			if(nbytes<0) {error_process(301);return;}
			alarm(0);
			struct PACKAGE *tmp;
			tmp=(struct PACKAGE *)buf;
			if(tmp->result==1)
			{
				user.result=1;
				write(sockfd,(char *)&user,sizeof(user));
				printf("successfully transfer\n");
				printf("whether choose to printlist:(yse is 1,no is 0)\n");
				int isprint;
				sacnf("%d",&isprint);
				if(isprint==1) printlist();
				return;
			}
			else if(tmp->result==2)
			{
				error_process(302);
				return;
			}
			else
			{
				error_process(303);
				return;
			}
			
		}
		else 
		{
			printf("your balance is not enough\n");
			continue;
		}
	}
}

void printlist()
{
	printf("%s\n",atmaddress );
	printf("%s\n",time );
	printf("%s\n",username);
	switch(user.servertype)
	{
		case 1:printf("savings amount:%u\n",user.money);
				break;
		case 2:printf("withdraws amount: %u\n",user.money);
				break;
		case 3:printf("transfer %u to %s\n",user.money,user.inaccount);
				break;
	}
	return;
}

void lookup()
{
	sigaction(SIGALRM,initsa(),NULL);
	user.servertype=4;
	write(sockfd,&user,sizeof(user));
	char buf[1024];
	bzero(buf,sizeof(buf));
	alarm(20);
	int nbytes=read(sockfd,buf,sizeof(buf));
	if(nbytes<0) {error_process(401);return;}
	alarm(0);
	struct PACKAGE *tmp;
	tmp=(struct PACKAGE *)buf;
	if(tmp->result==1) return;
	else {error_process(402);return;}
}

void login_menu()
{
	int loginsussceful=0;
	printf("------------------------------------------------------------\n");
	printf("|welcome to our bank,please input your account and password|\n");
	printf("------------------------------------------------------------\n");
	while(loginsussceful!=1)
	{	
		bzero(&user,sizeof(user));
		printf("account:\n");
		scanf("%s",user.account);
		printf("password:\n");
		scanf("%s",user.password);
		user.servertype=0;
		char buf[1024];
		bzero(buf,sizeof(buf));
		write(sockfd,&user,sizeof(user));
		read(sockfd,buf,sizeof(buf));
		struct PACKAGE *tmp;
		tmp=(struct PACKAGE *)buf;
		if(tmp->result==0)
		{
			printf("your account or password is incorrect,please input again\n");
			continue;
		}
		else
		{
			printf("login successful\n");
			loginsussceful=1;
		}
	}
	return;
}
int menu()
{
	printf("---------------------------------------------\n");
	printf("|Dear %s:                                   |\n",user.username);
	printf("|    please choose your process             |\n");
	printf("---------------------------------------------\n");
	printf("|    1.savings                              |\n");
	printf("|    2.withdraws                            |\n");
	printf("|    3.transfer                             |\n");
	printf("|    4.look up balance                      |\n");
	printf("|    5.print list                           |\n");
	printf("|    6.exit                                 |\n");
	printf("|    Please input your need!                |\n");
	printf("---------------------------------------------\n");
	int choose;
	scanf("%d",&choose);
	return choose;
}

int main()
{	
	init_mysql();
	struct sockaddr_in srvaddr;
	bzero(&srvaddr,sizeof(srvaddr));
	srvaddr.sin_family=AF_INET;
	srvaddr.sin_port=htons(PORT);
	inet_aton(SRVIP,&srvaddr.sin_addr);

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(socket<0) error_process(1);
	if(connect(sockfd,(struct sockaddr*)&srvaddr,sizeof(struct sockaddr))==-1) error_process(2);
	
	bzero(&user,sizeof(user));
	//user.atmaddress=MYADDRESS;
	strcpy(user.atmaddress,MYADDRESS);
	login_menu();
	while(1)
	{
		int choose=menu();
		switch(choose)
		{
			case 1: savings();break;
			case 2:	withdraws();break;
			case 3: transfer();break;
			case 4: lookup();
					printf("%s\n",tmp.username);
					printf("%u\n",tmp.money);
					printf("%s\n",tmp.atmaddress);
					printf("%s\n",tmp.time);
					break;
			case 5:	printlist();break;
			case 6:	exit(0);break;
			default :break;
		}
	}
}
