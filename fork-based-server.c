/*
 * fork-based-server.c
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
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <signal.h>
#include <math.h>
#include <sys/stat.h>
#include "protocol.h"

 int pids[MAX_NUM_OF_CLIENTS];
 int currentAmountOfTCPClient=0;

 struct client
 {
 int id;
 int samples[MAX_NUM_OF_SAMPLES];
 int index;
 long count;
 double avg;
 int isConnected;
 };

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

 int getIndexByValue(int value)
 {
 for(int i=0;i<MAX_NUM_OF_CLIENTS;i++)
 {
 if(pids[i]==value)
    return i;
 }
 return -1;
 }

 void sigchld_handler(int s)
 {
 int saved_errno = errno;
 int pid;
 int index;
 while((pid=waitpid(-1, NULL, WNOHANG))> 0)
 {
 index=getIndexByValue(pid);
 pids[index]=0;
 }
 errno = saved_errno;
 }

 int countTheConnectedClients(struct client * clients,int * clientsIds,double * clientsData,int command)
 {
 int connected=0;
 for(int i=0;i<MAX_NUM_OF_CLIENTS;i++)
 {
    if((clients[i].isConnected)==1)
    {
    clientsIds[connected]=clients[i].id;
    clientsData[connected]=command==WHOIS? clients[i].samples[(clients[i].index)-1]:clients[i].avg;
    connected++;
    }
 }
 return connected;
 }

 void getPidOfTheWorstClient(double * clientsData,int * clientsIds,int len,int *Id)
 {
 double max;
 int index=0;
 //put in max variable the first data of client whose has'nt been used
 while(index<len&&clientsIds[index]==-1)
    index++;
 max=clientsData[index];
 for(int i=index+1;i<len;i++)
 {
    if(clientsData[i]>max&&clientsIds[i]!=-1)
    {
       max=clientsData[i];
	   index=i;
	}
 }
 (*Id)=clientsIds[index];
 clientsIds[index]=-1;
 }

 int main(int argc,char *argv[])
 {
 int recvResult;
 int fileForSharedMemory;
 struct sockaddr_in server,client;
 struct sockaddr_un unServer,unClient;
 int new_fd,sockUDS,sockTCP;
 int len;
 const int port = 3344;
 int pid;
 int buf;
 int clientResult=0;
 long clientOverhead;
 struct flock lock;
 struct timeval begin,end;
 int differenceServer;
 struct sigaction sa;
 struct client * sharedDetailsOfClients;

//////////////////SOCK STREAM////////////////////
// if((sockUDS=socket(AF_UNIX,SOCK_STREAM,0))<0)
//{
//   perror("server: socketUDS");
//   exit(1);
//}
 if((sockUDS=socket(AF_UNIX,SOCK_DGRAM,0))<0)
 {
    perror("server: socketUDS");
	exit(1);
 }

 unServer.sun_family=AF_UNIX;
 strcpy(unServer.sun_path,SOCK_PATH);
 unlink(unServer.sun_path);
 len=strlen(unServer.sun_path)+sizeof(unServer.sun_family);

 if(bind(sockUDS,(struct sockaddr*)&unServer,len))
 {
    perror("server: bind");
    exit(1);
 }

//////////////////SOCK STREAM////////////////////
// if(listen(sockUDS,5)<0)
// {
//    perror("server: listen");
//    exit(1);
// }

 if((sockTCP=socket(AF_INET,SOCK_STREAM,0))<0)
 {
    perror("server: socketUDS");
	exit(1);
 }

 server.sin_family=AF_INET;
 server.sin_port=htons(port);
 server.sin_addr.s_addr=htonl(INADDR_ANY);

 if(bind(sockTCP,(struct sockaddr*)&server,sizeof(server))<0)
 {
    perror("server: bind");
    exit(1);
 }

 if(listen(sockTCP,5)<0)
 {
    perror("server: listen");
	exit(1);
 }

 if((fileForSharedMemory=open("dummyFile", O_RDWR | O_CREAT | O_TRUNC, 0666))<0)
 {
    perror("server: openSharedMemory");
	exit(1);
 }

 sharedDetailsOfClients = (struct client*)mmap(NULL, sizeof(struct client)*MAX_NUM_OF_CLIENTS, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED,fileForSharedMemory, 0);
 if(sharedDetailsOfClients == MAP_FAILED)
 {
    perror("server: mmap");
	exit(1);
 }
 pid=fork();
 if(pid<0)
 {
    perror("server: fork");
	exit(1);
 }
 if(pid==0)
 {
 unsigned int lens=sizeof(unClient);
//////////////////SOCK STREAM////////////////////
// for(;;)
// {
// new_fd=accept(sockUDS,(struct sockaddr*)&unClient, &lens);
// printf("new UDS client\n");
// if(new_fd==-1)
//    perror("server: acceptUDS");
// else
// {
 for(;;)
 {
 int command=0;
 int connectedClients=0;
 int dataForClient=0;
 int clientsIds[MAX_NUM_OF_CLIENTS] ;
 double clientsData[MAX_NUM_OF_CLIENTS] ;
//////////////////SOCK STREAM////////////////////
// if((recvResult=recvExtra(new_fd,&command,sizeof(int),0))<0)
//   perror("server: recvUDS");
// if(recvResult<=0)
//   break;
 if((recvResult=recvfrom(sockUDS, &command, sizeof(int), 0, (struct sockaddr*)&unClient, &lens))<0)
 {
    perror("server: recvfrom");
	break;
 }

 command=ntohl(command);
 lock.l_type=F_RDLCK;
 lock.l_start=0;
 lock.l_whence=SEEK_SET;
 lock.l_len=0;
 lock.l_pid=getpid();

 if(command==WHOIS)
 {
//get the lock
 if(fcntl(fileForSharedMemory,F_GETLK,&lock)<0)
 {
    perror("uds: fcntl");
	break;
 }

 if(lock.l_type==F_UNLCK)
 {
    lock.l_type=F_RDLCK;
	if(fcntl(fileForSharedMemory,F_SETLKW,&lock)<0)
	{
	   perror("uds: fcntl");
	   break;
    }
	connectedClients=countTheConnectedClients(sharedDetailsOfClients,clientsIds,clientsData,command);
   	//free the lock
	lock.l_type=F_UNLCK;
	if(fcntl(fileForSharedMemory,F_SETLK,&lock)<0)
	{
       perror("fcntl: freeUdsServer");
	   break;
	}
 }
 connectedClients=htonl(connectedClients);
//////////////////SOCK STREAM////////////////////
//send the num of clients
// if(sendExtra(new_fd,&connectedClients, sizeof(int), 0)<0)
//    perror("udsServer send");
 if(sendto(sockUDS, &connectedClients, sizeof(int), 0, (struct sockaddr*)&unClient, lens)<0)
    perror("server: sendTo");
 connectedClients=ntohl(connectedClients);
 for(int i=0;i<connectedClients;i++)
 {
//send the id of current connected client
 dataForClient=clientsIds[i];
 dataForClient=htonl(dataForClient);
//////////////////SOCK STREAM////////////////////
// if(sendExtra(new_fd,&dataForClient,sizeof(int),0)<0)
//    perror("udsServer: send");
 if(sendto(sockUDS, &dataForClient, sizeof(int), 0, (struct sockaddr*)&unClient, lens)<0)
    perror("server: sendTo");
//send the last sampple..(idex is the next empty cell in samples array)
 dataForClient=clientsData[i];
 dataForClient=htonl(dataForClient);
//////////////////SOCK STREAM////////////////////
// if(sendExtra(new_fd,&dataForClient,sizeof(int),0)<0)
//    perror("udsServer: send");
 if(sendto(sockUDS, &dataForClient, sizeof(int), 0, (struct sockaddr*)&unClient, lens)<0)
    perror("server: sendTo");
 }
 }//WHOIS
 else if(command==GETWORST)
 {
 double computation;
 int clientsForSend;
 if(fcntl(fileForSharedMemory,F_GETLK,&lock)<0)
 {
	perror("uds: fcntl");
    break;
 }
 if(lock.l_type==F_UNLCK)
 {
 lock.l_type=F_RDLCK;
 if(fcntl(fileForSharedMemory,F_SETLKW,&lock)<0)
 {
   perror("uds: fcntl");
   break;
 }
 connectedClients=countTheConnectedClients(sharedDetailsOfClients, clientsIds, clientsData,GETWORST);
//free the lock
 lock.l_type=F_UNLCK;
 if(fcntl(fileForSharedMemory,F_SETLK,&lock)<0)
 {
    perror("fcntl: freeUdsServer");
    break;
 }
 }
//the ceil action
 computation=0.5*connectedClients;
 clientsForSend=0.5*connectedClients;
 if(((double)clientsForSend)<computation)
  clientsForSend++;
 clientsForSend=htonl(clientsForSend);
//////////////////SOCK STREAM////////////////////
// if(sendExtra(new_fd,&clientsForSend, sizeof(int), 0)<0)
//    perror("udsServer send");
 if(sendto(sockUDS, &clientsForSend, sizeof(int), 0, (struct sockaddr*)&unClient, lens)<0)
    perror("server: sendTo");
 clientsForSend=ntohl(clientsForSend);
 for(int i=0;i<clientsForSend;i++)
 {
 getPidOfTheWorstClient(clientsData, clientsIds, connectedClients, &dataForClient);
 dataForClient=htonl(dataForClient);
//////////////////SOCK STREAM////////////////////
//if(sendExtra(new_fd,&dataForClient, sizeof(int), 0)<0)
//   perror("udsServer send");
 if(sendto(sockUDS, &dataForClient, sizeof(int), 0, (struct sockaddr*)&unClient, lens)<0)
    perror("server: sendTo");
 }
 }//GETWORST
 else if(command==GREP)
 {
 char * grepResult;
 int pid;
 int file;
 int pfds[2];
 int j;
 int numOfBytes;
 char buff[MAX_DATA_SIZE];
 pid=fork();
 if(pid<0)
 {
    perror("server: forkSTDout");
    break;
 }
 if(pid==0)
 {
 if((file=open("clientsSchema",O_RDWR| O_CREAT | O_TRUNC, 0666))<0)
 {
    perror("server: open");
	exit(1);
 }
 if(dup2(file,STDOUT_FILENO)<0)
 {
    perror("server: dup1");
    exit(1);
 }
 lock.l_pid=getpid();
 if(fcntl(fileForSharedMemory,F_GETLK,&lock)<0)
 {
    perror("uds: fcntl");
    exit(1);
 }
 if(lock.l_type==F_UNLCK)
 {
 lock.l_type=F_RDLCK;
 if(fcntl(fileForSharedMemory,F_SETLKW,&lock)<0)
 {
    perror("uds: fcntl");
    exit(1);
 }
 for(int i=0;i<MAX_NUM_OF_CLIENTS;i++)
 {
 printf("%s%d%c","Client num:",sharedDetailsOfClients[i].id,' ');
 for(j=0;j<MAX_NUM_OF_SAMPLES;j++)
    printf("%d%c",sharedDetailsOfClients[i].samples[j],' ');
 printf("%s%lf%c","The avg:",sharedDetailsOfClients[i].avg,'\n');
 }
 printf("%c",'\n');
//free the lock
 lock.l_type=F_UNLCK;
 if(fcntl(fileForSharedMemory,F_SETLK,&lock)<0)
 {
    perror("fcntl: freeUdsServer");
    exit(1);
 }
 }
 close(file);
 exit(0);
 }//CHILD PROCESS
 else //PARENT 1
 {
 int status2;
 waitpid(pid, &status2, 0);
 if(status2==1)
  {
     perror("server: grep");
     break;
  }
 if((recvResult=recvfrom(sockUDS, &numOfBytes, sizeof(int), 0, (struct sockaddr*)&unClient, &lens))<0)
 {
    perror("server: recvfrom");
    break;
 }
 numOfBytes=ntohl(numOfBytes);
 if((recvResult=recvfrom(sockUDS, buff, numOfBytes, 0, (struct sockaddr*)&unClient, &lens))<0)
 {
    perror("server: recvfrom");
    break;
 }
 buff[numOfBytes]='\0';
 pipe(pfds);
 pid=fork();
 if(pid<0)
 {
    perror("server: forkSTDout");
    break;
 }
 if(pid==0)
 {
 close(pfds[0]);
 if((file=open("clientsSchema",O_RDONLY, 0666))<0)
 {
	 perror("server: open");
	 exit(1);
 }
 if((dup2(file,STDIN_FILENO))<0)
 {
    perror("server :dup2");
    exit(1);
 }

 if((dup2(pfds[1],STDOUT_FILENO))<0)
 {
	perror("server :dup2");
	exit(1);
 }
 char* args[]={"/bin/grep",buff,NULL};
 if(execve(args[0],args,NULL)<0)
 {
    perror("serever : execve");
    exit(1);
 }
 }//CHILD PROCESS
 else
 {
 int status,size;
 waitpid(pid,&status,0);
 if(status==1)
 {
    perror("server: grep");
    break;
 }
 close(pfds[1]);
 char buffer[2000];
 if((dup2(pfds[0],STDIN_FILENO))<0)
 {
    perror("server :dup2");
    break;
 }
 scanf("%[^\t]s", buffer);
 size=strlen(buffer);
 buffer[size]='\0';
 size=htonl(size);
 if(sendto(sockUDS, &size, sizeof(int), 0, (struct sockaddr*)&unClient, lens)<0)
    perror("server: sendTo");
 size=ntohl(size);
 if(sendto(sockUDS, buffer, size, 0, (struct sockaddr*)&unClient, lens)<0)
    perror("server: sendTo");
 close(pfds[0]);
 }//PARENT 2
 }//PARENT 1
 }
 }
//////////////////SOCK STREAM////////////////////
//}
//close(new_fd);
//}
 close(sockUDS);
 exit(0);
 }
 else
 {
 sa.sa_handler=sigchld_handler;
 sigemptyset(&sa.sa_mask);
 sa.sa_flags=SA_RESTART;
 if (sigaction(SIGCHLD, &sa, NULL) == -1)
 {
    perror("sigaction");
    exit(1);
 }
 int currentIndex;
 ////////////TCP clients//////////////
 for(;;)
 {
 unsigned int lenc=sizeof(client);
 if((new_fd=accept(sockTCP,(struct sockaddr*)&client,&lenc))<0)
 {
    perror("server: accept");
	exit(1);
 }
 printf("%s%d%c","New tcp client ",new_fd+currentAmountOfTCPClient+1,'\n');
 currentIndex=getIndexByValue(0);
 currentAmountOfTCPClient++;
 pids[currentIndex]=fork();
 if(pids[currentIndex]<0)
 {
    perror("server: fork");
    break;
 }
 if(pids[currentIndex]==0)
 {
 int index;
 double avg;
 close(sockTCP);
 lock.l_type=F_WRLCK;
 lock.l_start=(currentIndex)*sizeof(client);
 lock.l_whence=SEEK_SET;
 lock.l_len=sizeof(client);
 lock.l_pid=getpid();
 if(fcntl(fileForSharedMemory,F_GETLK,&lock)<0)
 {
    perror("sever: fcntl");
	break;
 }
 if(lock.l_type==F_UNLCK)
 {
 lock.l_type=F_WRLCK;
 if(fcntl(fileForSharedMemory,F_SETLKW,&lock)<0)
 {
	 perror("server: fcntl");
	 break;
 }
 sharedDetailsOfClients[currentIndex].id=new_fd+currentAmountOfTCPClient;
 sharedDetailsOfClients[currentIndex].index=0;
 sharedDetailsOfClients[currentIndex].count=0;
 sharedDetailsOfClients[currentIndex].avg=0;
 sharedDetailsOfClients[currentIndex].isConnected=1;
 lock.l_type=F_UNLCK;
 if(fcntl(fileForSharedMemory,F_SETLK,&lock)<0)
 {
	 perror("server: fcntl");
	 break;
 }
 }
 for(;;)
 {
 if((recvResult=recvExtra(new_fd,&buf,sizeof(int),0))<0)
    perror("server: recvTCP1");
  if(recvResult==0)
    break;
 buf=ntohl(buf);
 gettimeofday(&begin, 0);
 if((recvResult=recvExtra(new_fd,&clientResult,sizeof(int),0))<0)
    perror("server: recvTCP2");
 if(recvResult<=0)
    break;
 clientResult=ntohl(clientResult);
 gettimeofday(&end, 0);
 differenceServer=(1000000 * end.tv_sec + end.tv_usec) - (1000000 * begin.tv_sec + begin.tv_usec);
 clientOverhead=differenceServer-clientResult;
 lock.l_type=F_WRLCK;
 lock.l_start=(currentIndex)*sizeof(client);
 lock.l_whence=SEEK_SET;
 lock.l_len=sizeof(client);
 lock.l_pid=getpid();
 if(fcntl(fileForSharedMemory,F_GETLK,&lock)<0)
 {
	 perror("sever: fcntl");
	 break;
 }
 if(lock.l_type==F_UNLCK)
 {
 lock.l_type=F_WRLCK;
 if(fcntl(fileForSharedMemory,F_SETLKW,&lock)<0)
 {
	 perror("server: fcntl");
	 break;
 }
 index=sharedDetailsOfClients[currentIndex].index;
 avg=sharedDetailsOfClients[currentIndex].avg;
 sharedDetailsOfClients[currentIndex].samples[index]=clientOverhead;
 index=(index+1)%MAX_NUM_OF_SAMPLES;
 sharedDetailsOfClients[currentIndex].index=index;
 sharedDetailsOfClients[currentIndex].count++;
 avg=(avg*(sharedDetailsOfClients[currentIndex].count-1))+clientOverhead;
 avg/=sharedDetailsOfClients[currentIndex].count;
 sharedDetailsOfClients[currentIndex].avg=avg;
 printf("%s%d%s%d%c","client: ",new_fd+currentAmountOfTCPClient," over ",clientOverhead,'\n');
//free the lock
 lock.l_type=F_UNLCK;
 if(fcntl(fileForSharedMemory,F_SETLK,&lock)<0)
 {
	perror("server: fcntl");
	break;
 }
 }
 }//FOR;;
 lock.l_type=F_WRLCK;
 if(fcntl(fileForSharedMemory,F_GETLK,&lock)<0)
 {
	 perror("sever: 1fcntl");
	 break;
 }
 if(lock.l_type==F_UNLCK)
 {
 lock.l_type=F_WRLCK;
 if(fcntl(fileForSharedMemory,F_SETLKW,&lock)<0)
 {
	 perror("server: 2fcntl");
	 break;
 }
 sharedDetailsOfClients[currentIndex].isConnected=0;
//free the lock
 lock.l_type=F_UNLCK;
 if(fcntl(fileForSharedMemory,F_SETLK,&lock)<0)
 {
	 perror("server: 3fcntl");
	 break;
 }

 close(new_fd);
 exit(0);
 }
 }//CHILD PROCESS
 else
 {
  close(new_fd);
 }//PARENT
 }
 }
 return 0;
 }
