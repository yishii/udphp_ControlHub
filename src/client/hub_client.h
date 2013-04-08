#ifndef __HUB_CLIENT_H__
#define __HUB_CLIENT_H__

typedef enum {
    HUBC_OK = 1,
    HUBC_ERR
} HubResult;

HubResult startMessageReceivingThread(int port);
HubResult startHubConnectionMasterThread(char* server_url,int server_port,char* username,char* password,char* connect_to,int connection_port,bool resolve_host);
HubResult startHubConnectionSlaveThread(char* server_url,int server_port,char* username,char* password,int connection_port,bool resolve_host);

#endif /* __HUB_CLIENT_H__ */
