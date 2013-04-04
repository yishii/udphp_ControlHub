/*
  connection client sample

  ver 0.1,3/31/2013
  
  Copyright by Yasuhiro ISHII,2013
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <alloca.h>
#include <errno.h>
#include "hub_client.h"

void* hubAccessingMasterThread(void* arg);
void* hubAccessingSlaveThread(void* arg);
void* messageReceivingThread(void* arg);
bool openTcpConnection(int* socket,char* url,int port);
bool receiveTcpMessage(int sock,char* msg,int len);
bool sendTcpMessage(int sock,char* msg);
bool receiveTcpResponse(int sock,char* msg);
bool removeNewline(char* buff);
bool loginServer(int sock,char* username,char* password);
bool commandStat(int sock,char* user);
bool commandPoll(int sock,char* user);
bool commandGetip(int sock,char* user,char* ip);
bool commandConnectTo(int sock,char* user);
bool sendUdpMessage(char* url,int port,char* msg,int len);
bool getAddress(char* hostname,char* ipstr,int ipstr_len);

struct serverinfo {
    char* url;
    int hub_tcp_port;
    char* username;
    char* password;
    char* connect_to;
    int connection_udp_port;
};

HubResult startMessageReceivingThread(int port)
{
    pthread_t t1;
    HubResult ret = HUBC_OK;

    pthread_create(&t1,NULL,messageReceivingThread,(void*)&port);

    sleep(1);

    //pthread_join(t1,NULL);

    return(ret);
}

HubResult startHubConnectionMasterThread(char* server_url,int server_port,char* username,char* password,char* connect_to,int connection_port)
{
    pthread_t t1;
    struct serverinfo info;
    HubResult ret = HUBC_OK;

    char ip[20];
    if(getAddress(server_url,ip,sizeof(ip)) == false){
	printf("%s : Address resolve error\n",__func__);
	return(HUBC_ERR);
    }

    memset(&info,0,sizeof(struct serverinfo));

    info.url = ip;
    info.hub_tcp_port = server_port;
    info.username = username;
    info.password = password;
    info.connect_to = connect_to;
    info.connection_udp_port = connection_port;

    pthread_create(&t1,NULL,hubAccessingMasterThread,(void*)&info);

    pthread_join(t1,NULL);

    return(ret);
}

HubResult startHubConnectionSlaveThread(char* server_url,int server_port,char* username,char* password,int connection_port)
{
    pthread_t t1;
    struct serverinfo info;
    HubResult ret = HUBC_OK;

    char ip[20];
    if(getAddress(server_url,ip,sizeof(ip)) == false){
	printf("%s : Address resolve error\n",__func__);
	return(HUBC_ERR);
    }

    memset(&info,0,sizeof(struct serverinfo));

    info.url = ip;
    info.hub_tcp_port = server_port;
    info.username = username;
    info.password = password;
    info.connect_to = NULL;
    info.connection_udp_port = connection_port;

    pthread_create(&t1,NULL,hubAccessingSlaveThread,(void*)&info);

    pthread_join(t1,NULL);

    return(ret);
}

void* hubAccessingMasterThread(void* arg)
{
    int i;
    int sock;
    bool result;
    char msg[100];
    char connect_to_ip[20];
    struct serverinfo* info = arg;
    const char *dummyAccessMessage = "Hello";

    printf("[%s] url=[%s],hub_tcp_port=[%d],username=[%s],password=[%s],connect_to=[%s],connection_udp_port=[%d]\n",
	   __func__,
	   info->url,
	   info->hub_tcp_port,
	   info->username,
	   info->password,
	   info->connect_to,
	   info->connection_udp_port);

    result = openTcpConnection(&sock,info->url,info->hub_tcp_port);

    if(result == false){
	printf("%s : openTcpConnection failed\n",__func__);
	return(NULL);
    }

    result = loginServer(sock,info->username,info->password);

    if(result == true){
	printf("**************************************************\n");
	printf("Authorization finished\n");
	printf("**************************************************\n");
    } else {
	return(NULL);
    }

    for(i=0;;i++){
	if(commandStat(sock,info->connect_to) == true){
	    printf("account is logged in\n");
	    break;
	} else {
	    printf("account is not logged in\n");
	}

	sleep(2);
    }

    commandGetip(sock,info->connect_to,connect_to_ip);

    printf("Target IP : [%s]\n",connect_to_ip);

    sendUdpMessage(connect_to_ip,info->connection_udp_port,(char*)dummyAccessMessage,strlen(dummyAccessMessage));
    printf("%s:Sent dummy access message\n",__func__);

    commandConnectTo(sock,info->connect_to);

    while(1){
	printf("[%s] sleep...\n",__FUNCTION__);
	sleep(2);
    }

    close(socket);
    return(NULL);
}

void* hubAccessingSlaveThread(void* arg)
{
    int i;
    int sock;
    bool result;
    char buff[100];
    char requested_user[20];
    char requested_user_ip[20];
    struct serverinfo* info = arg;

    printf("[%s] url=[%s],hub_tcp_port=[%d],username=[%s],password=[%s],connection_udp_port=[%d]\n",
	   __func__,
	   info->url,
	   info->hub_tcp_port,
	   info->username,
	   info->password,
	   info->connection_udp_port);

    result = openTcpConnection(&sock,info->url,info->hub_tcp_port);

    if(result == false){
	printf("%s : openTcpConnection failed\n",__func__);
	return(NULL);
    }

    result = loginServer(sock,info->username,info->password);

    if(result == true){
	printf("**************************************************\n");
	printf("Authorization finished\n");
	printf("**************************************************\n");
    } else {
	return(NULL);
    }

    for(i=0;;i++){
	if(commandPoll(sock,requested_user) == false){
	    printf("Nothing is happened by response of POLL command\n");
	} else {
	    printf("Requested from [%s]\n",requested_user);
	    break;
	}
	sleep(2);
    }
    
    commandGetip(sock,requested_user,requested_user_ip);

    while(1){
	sleep(2);
	sendUdpMessage(requested_user_ip,info->connection_udp_port,"Hello,from slave thread",strlen("Hello,from slave thread"));
	printf("[%s] sent udp message,and sleep...\n",__FUNCTION__);
    }

    close(socket);
    return(NULL);
}

bool loginServer(int sock,char* username,char* password)
{
    bool ret = false;
    char msg[100];

    receiveTcpResponse(sock,"CONNECT");

    sprintf(msg,"USER %s\r\n",username);
    sendTcpMessage(sock,msg);

    receiveTcpResponse(sock,"OK");
    sleep(1);

    sprintf(msg,"PASS %s\r\n",password);
    sendTcpMessage(sock,msg);

    receiveTcpResponse(sock,"OK");
    
    if(receiveTcpResponse(sock,"CONNECTED") == true){
	ret = true;
    } else {
	printf("Login error\n");
    }

    return(ret);
}

bool removeNewline(char* buff){
    char* pos;
    bool ret = false;

    if((pos = strrchr(buff,'\n')) != NULL){
	*pos = '\0';
	ret = true;
    }
    if((pos = strrchr(buff,'\r')) != NULL){
	*pos = '\0';
	ret = true;
    }

    return(ret);
}

bool commandStat(int sock,char* user)
{
    char buff[100];
    bool ret = false;

    sprintf(buff,"STAT %s\r\n",user);
    sendTcpMessage(sock,buff);

    removeNewline(buff);
    printf("%s : send [%s]\n",__func__,buff);
    
    receiveTcpMessage(sock,buff,sizeof(buff));

    if(*buff == '1'){
	ret = true;
    }

    return(ret);
}

bool commandPoll(int sock,char* user)
{
    char buff[100];
    bool ret = false;

    sendTcpMessage(sock,"POLL\r\n");

    
    receiveTcpMessage(sock,buff,sizeof(buff));
    removeNewline(buff);
    printf("%s : response of POLL : [%s]\n",__func__,buff);

    if(*buff != '*'){
	ret = true;
	removeNewline(buff);
	strcpy(user,buff);
    }

    return(ret);
}

bool commandGetip(int sock,char* user,char* ip)
{
    char buff[100];
    bool ret = false;

    sprintf(buff,"GETIP %s\r\n",user);
    sendTcpMessage(sock,buff);
    
    receiveTcpMessage(sock,buff,sizeof(buff));
    printf("%s : response of GETIP : [%s]\n",__func__,buff);

    if(*buff != '*'){
	ret = true;
	removeNewline(buff);
	strcpy(ip,buff);
    }

    return(ret);
}

bool commandConnectTo(int sock,char* user)
{
    char buff[100];
    bool ret = false;

    sprintf(buff,"CONNECTTO %s\r\n",user);
    sendTcpMessage(sock,buff);
    
    receiveTcpMessage(sock,buff,sizeof(buff));

    if(*buff == '1'){
	ret = true;
    }

    return(ret);
}

bool receiveTcpResponse(int sock,char* msg)
{
    char* buff;
    bool result;
    bool ret = false;

    buff = alloca(100);
    result = receiveTcpMessage(sock,buff,100);

    removeNewline(buff);

    if(result == true){
	printf("%s : received message[%s]\n",__FUNCTION__,buff);
	if(strcmp(buff,msg) == 0){
	    ret = true;
	} else {
	    printf("response is not what I expected(expected=[%s],received=[%s])\n",msg,buff);
	}
    }
    return(ret);
}

bool receiveTcpMessage(int sock,char* msg,int len)
{
    int result;

    result = read(sock,msg,len);

    if(result == -1){
	return(false);
    } else {
	return(true);
    }
}

bool sendTcpMessage(int sock,char* msg)
{
    printf("%s\n",__FUNCTION__);

    printf("\x1b[31m[%s]\x1b[m\n",msg);
    if(send(sock,msg,strlen(msg),0) == -1){
	printf("send failed,errno = %d\n",errno);
	return(false);
    } else {
	return(true);
    }
}

bool openTcpConnection(int* sock,char* url,int port)
{
    struct sockaddr_in dest_address;
    int dest_socket;
    int result;

    dest_socket = socket(AF_INET,SOCK_STREAM,0);

    memset((void*)&dest_address,0,sizeof(dest_address));

    dest_address.sin_family = AF_INET;
    dest_address.sin_port = htons(port);
    dest_address.sin_addr.s_addr = inet_addr(url);
    
    printf("Connect to %s:%d : ",url,port);

    result = connect(dest_socket,(struct sockaddr*)&dest_address,sizeof(dest_address));
    if(result == 0){
	printf("Connected\n");
	*sock = dest_socket;
	return(true);
    } else {
	printf("Failed\n");
    }

    return(false);
}

void* messageReceivingThread(void* arg)
{
    int sock;
    struct sockaddr_in addr;
    char buff[1024*10];
    ssize_t size;
    int port = *((int*)arg);

    printf("[%s] port = %d\n",__func__,port);

    sock = socket(AF_INET,SOCK_DGRAM,0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock,(struct sockaddr*)&addr,sizeof(addr));

    memset(buff,0,sizeof(buff));
    while(recv(sock,buff,sizeof(buff),0) > 0){
	printf("%s:received:[%s]\n",__func__,buff);
    }

    printf("thread[%s] end\n",__func__);

    return(NULL);
}

bool sendUdpMessage(char* url,int port,char* msg,int len)
{
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET,SOCK_DGRAM,0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(url);

    sendto(sock,msg,len,0,(struct sockaddr *)&addr,sizeof(addr));
    close(sock);

    return(true);
}

bool getAddress(char* hostname,char* ipstr,int ipstr_len)
{
    char* service = "http";
    struct addrinfo hints,*res0,*res;
    int err;
    int sock;
    bool found = false;

    memset(&hints,0,sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = PF_UNSPEC;

    if((err = getaddrinfo(hostname,service,&hints,&res0)) != 0){
	printf("err %d\n",err);
	return false;
    }

    for(res=res0;res!=NULL;res=res->ai_next){
	sock = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
	if(sock < 0){
	    continue;
	}

	if(connect(sock,res->ai_addr,res->ai_addrlen) != 0){
	    close(sock);
	    continue;
	}
	found = true;
	
	break;
    }

    if(inet_ntop(AF_INET,&((struct sockaddr_in*)(res->ai_addr))->sin_addr,ipstr,ipstr_len) == NULL){
	printf("inet_ntop returns error\n");
    }	

    freeaddrinfo(res0);

    return(found);
}
