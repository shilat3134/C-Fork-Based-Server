/*
 * uds-client.c
 *
 *  Created on: Dec 4, 2017
 *      Author: user
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "protocol.h"


 void printInstructions()
 {
 printf("To see all connected clients with their last overhead type whois\n");
 printf("To see the 5 percent clients with the worst avg type getworst\n");
 printf("To search for pettern type grep\n");
 }

 int recvExtra(int fd,void * buf,int len,int flags)
 {
 int result=0;
 int numOfReceivedBytes=0;
 char * cursor=buf;
 do {
 result=recv(fd,cursor,len,flags);
 if(result==0||result==-1)
    return result;
 numOfReceivedBytes+=result;
 cursor+=result;
 } while (numOfReceivedBytes<len);
 return numOfReceivedBytes;
 }

 int sendExtra(int fd,void * buf,int len,int flags)
 {
 int result=0;
 int numOfSentBytes=0;
 char * cursor=buf;
 do {
 result=send(fd,cursor,len,flags);
 if(result==-1)
 return result;
 numOfSentBytes+=result;
 cursor+=result;
 } while (numOfSentBytes<len);
 return numOfSentBytes;
 }

 int main(int argc, char *argv[])
 {
 int sockfd;
 struct sockaddr_un server,server2;
 int len;
 int stringLen;
//////////////////SOCK STREAM////////////////////
// if((sockfd=socket(AF_UNIX,SOCK_STREAM,0))<0)
// {
// 	 perror("client: socket");
// 	 exit(1);
// }

 if((sockfd=socket(AF_UNIX,SOCK_DGRAM,0))<0)
 {
    perror("client: socket");
  	exit(1);
 }

 server.sun_family=AF_UNIX;
 strcpy(server.sun_path,SOCK_PATH2);
 len=strlen(server.sun_path)+sizeof(server.sun_family);
 unlink(server.sun_path);

 server2.sun_family=AF_UNIX;
 strcpy(server2.sun_path,SOCK_PATH);
 len=strlen(server2.sun_path)+sizeof(server2.sun_family);

 if(bind(sockfd, (struct sockaddr *)&server, sizeof(server))<0)
 {
    perror("udsClient: bind");
	exit(1);
 }
//////////////////SOCK STREAM////////////////////
//  if (connect(sockfd,(struct sockaddr*)&server,len)<0)
//  {
//     close(sockfd);
//     perror("client: connect");
//     exit(1);
//  }

 printInstructions();
 for(;;)
 {
  	int recvResult;
  	int command;
  	int serverNumOfData=0;
  	int serverData=0;
  	char buf[20];
  	scanf("%s",buf);
    if(strcmp(buf,"whois")==0)
  	{
  	   command=WHOIS;
  	   command=htonl(command);
//////////////////SOCK STREAM////////////////////
// if(sendExtra(sockfd,&command,sizeof(int),0)<0)
//    perror("client: send");
    if(sendto(sockfd, &command, sizeof(int), 0, (struct sockaddr*)&server2,len)<0)
  	   perror("client: sendTo");
//////////////////SOCK STREAM////////////////////
// if((recvResult=recvExtra(sockfd, &serverNumOfData, sizeof(int), 0))<0)
//    perror("client: recv");
//    if(recvResult==0)
//    {
//	   printf("The connection with the server failed!\n");
//	   break;
//    }
    if((recvResult=recvfrom(sockfd, &serverNumOfData, sizeof(int), 0, NULL,NULL ))<0)
  	   perror("client: recfFrom");
    serverNumOfData=ntohl(serverNumOfData);
    printf("%s%d%c","The num of clients in result: ",serverNumOfData,'\n');
    for(int i=0;i<serverNumOfData;i++)
  	{
//////////////////SOCK STREAM////////////////////
// if((recvResult=recvExtra(sockfd,&serverData,sizeof(int),0))<0)
//   perror("recv client fd");
//   if(recvResult<=0)
//     	{
//    	   printf("the connection with the server failed!\n");
//     	   break;
//     	}
    if((recvResult=recvfrom(sockfd, &serverData, sizeof(int), 0, NULL, NULL))<0)
        perror("client: recfFrom");
    serverData=ntohl(serverData);
    printf("%s%d%c","The fd: ",serverData,'\n');
//////////////////SOCK STREAM////////////////////
//  if((recvResult=recvExtra(sockfd,&serverData,sizeof(int),0))<0)
//     perror("recv last sample");
//    if(recvResult<=0)
//        {
//      	   printf("the connection with the server failed!\n");
//      	   break;
//      	}
    if((recvResult=recvfrom(sockfd, &serverData, sizeof(int), 0,NULL,NULL))<0)
  	   perror("client: recfFrom");
    serverData=ntohl(serverData);
    printf("%s%d%c","The last sample: ",serverData,'\n');
  	}//FOR
    }//WHOIS
    else if(strcmp(buf,"getworst")==0)
    {
  	command=GETWORST;
  	command=htonl(command);
//////////////////SOCK STREAM////////////////////
//  if(sendExtra(sockfd,&command,sizeof(int),0)<0)
//     perror("client: send");
    if(sendto(sockfd, &command, sizeof(int), 0, (struct sockaddr*)&server2,len)<0)
  	   perror("client: sendTo");
//////////////////SOCK STREAM////////////////////
//  if((recvResult=recvExtra(sockfd, &serverNumOfData, sizeof(int), 0))<0)
//     perror("client: recv");
//    if(recvResult<=0)
//        {
//      	printf("the connection with the server failed!\n");
//      	break;
//    	}
    if((recvResult=recvfrom(sockfd, &serverNumOfData, sizeof(int), 0, NULL, NULL))<0)
  	   perror("client: recfFrom");
  	serverNumOfData=ntohl(serverNumOfData);
    printf("%s%d%c","The num of clients in result: ",serverNumOfData,'\n');
    for(int i=0;i<serverNumOfData;i++)
  	{
//////////////////SOCK STREAM////////////////////
// if((recvResult=recvExtra(sockfd,&serverData,sizeof(int),0))<0)
// 	 perror("recv client fd");
//    	if(recvResult<=0)
//    	  	{
//    	  	printf("the connection with the server failed!\n");
//    	  	break;
//    	  	}
    if((recvResult=recvfrom(sockfd, &serverData, sizeof(int), 0,NULL,NULL))<0)
  	   perror("client: recfFrom");
    serverData=ntohl(serverData);
    printf("%s%d%c","The fd: ",serverData,'\n');
    }//FOR
    }//GETWORST
    else if(strcmp(buf,"grep")==0)
  	{
  	char * grepResult;
  	command=GREP;
  	command=htonl(command);
  	if(sendto(sockfd, &command, sizeof(int), 0, (struct sockaddr*)&server2,len)<0)
  	   perror("client: sendTo");
  	printf("Enter pattern for grep: \n");
  	scanf("%s",buf);
    stringLen=strlen(buf);
    stringLen=htonl(stringLen);
    if(sendto(sockfd, &stringLen, sizeof(int), 0, (struct sockaddr*)&server2,len)<0)
  	   perror("client: sendTo");
    if(sendto(sockfd, &buf, strlen(buf), 0, (struct sockaddr*)&server2,len)<0)
  	   perror("client: sendTo");
	if((recvResult=recvfrom(sockfd, &serverNumOfData, sizeof(int), 0, NULL, NULL))<0)
       perror("client: recfFrom");
    serverNumOfData=ntohl(serverNumOfData);
    if((grepResult=(char*)malloc(serverNumOfData+1))==0)
	{
	   perror("server: malloc");
	   exit(1);
    }
    if((recvResult=recvfrom(sockfd, grepResult, serverNumOfData, 0, NULL, NULL))<0)
       perror("client: recfFrom");
    if(serverNumOfData>1)
    {
       grepResult[serverNumOfData]='\0';
       printf("the result of grep is: \n");
       printf("%s%c",grepResult,'\n');
    }
    free(grepResult);
    grepResult=NULL;
  	}//GREP
    else if(strcmp(buf,"x")==0)
       break;
    else
  	{
  	printf("wrong input\n");
  	printInstructions();
  	}
 }//FOR
 close(sockfd);
 return 0;
 }
