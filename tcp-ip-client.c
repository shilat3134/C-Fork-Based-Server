/*
 * tcp-ip-client.c
 *
 *  Created on: Nov 28, 2017
 *      Author: user
 */


#include <time.h>
#include <sys/time.h>
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
#include "protocol.h"

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
 int sum;
 const int port = 3344;
 char * serverIP="127.0.0.1";
 struct sockaddr_in server;
 struct timeval begin,end;
 if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
 {
    perror("client: socket");
	exit(1);
 }
 server.sin_family=AF_INET;
 server.sin_port=htons(port);
 server.sin_addr.s_addr=inet_addr(serverIP);

 if (connect(sockfd,(struct sockaddr*)&server,sizeof(server))<0)
 {
    close(sockfd);
    perror("tcpclient: connect");
    exit(1);
 }

 while(1)
 {
	int command=START;
	command=htonl(command);
	if(sendExtra(sockfd,&command,sizeof(int),0)<0)
	   perror("tcpclient: send");
	gettimeofday(&begin, 0);
	for(int i=0;i<1000000;i++)
	   sum+=i;
	gettimeofday(&end, 0);
	int computationTime= (1000000 * end.tv_sec + end.tv_usec) - (1000000 * begin.tv_sec + begin.tv_usec);
    computationTime=htonl(computationTime);
	if(sendExtra(sockfd,&computationTime,sizeof(int),0)<0)
	   perror("client: send");
	sleep(3);
 }
 close(sockfd);
 return 0;
 }
