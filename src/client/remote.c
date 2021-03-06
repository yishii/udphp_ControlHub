#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hub_client.h"

#define	HUB_SERVER_URL		"www.yishii.jp"
//#define	HUB_SERVER_URL	"49.212.150.105"
//#define	HUB_SERVER_URL	"127.0.0.1"
#define	HUB_SERVER_PORT		12800
#define	HUB_ACCOUNT_USERNAME	"user0"
#define	HUB_ACCOUNT_PASSWORD	"pass0"
#define HUB_ACCOUNT_TO_CONNECT	"user1"

#define CONNECTION_PORT		20010

int main(int argc,char** argv)
{
    HubResult result;

    startMessageReceivingThread(CONNECTION_PORT);

    result = startHubConnectionMasterThread(
	HUB_SERVER_URL,
	HUB_SERVER_PORT,
	HUB_ACCOUNT_USERNAME,
	HUB_ACCOUNT_PASSWORD,
	HUB_ACCOUNT_TO_CONNECT,
	CONNECTION_PORT,
	true);
}


