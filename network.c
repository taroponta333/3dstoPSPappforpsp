#include <pspkernel.h>
#include <pspdebug.h>

#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>

#include <stdio.h>
#include <string.h>

#include "network.h"

/*=========================================================
    ネットワーク初期化
=========================================================*/

int Network_Init(void)
{
    int ret;

    ret = sceNetInit(
        128 * 1024,
        42,
        4 * 1024,
        42,
        4 * 1024);

    if(ret < 0)
        return ret;

    ret = sceNetInetInit();

    if(ret < 0)
        return ret;

    ret = sceNetResolverInit();

    if(ret < 0)
        return ret;

    ret = sceNetApctlInit(
        0x1800,
        48);

    if(ret < 0)
        return ret;

    return 0;
}

/*=========================================================
    ネットワーク終了
=========================================================*/

void Network_Shutdown(void)
{
    sceNetApctlTerm();

    sceNetResolverTerm();

    sceNetInetTerm();

    sceNetTerm();
}

/*=========================================================
    接続確認
=========================================================*/

int Network_IsConnected(void)
{
    int state;

    if(sceNetApctlGetState(&state) < 0)
        return 0;

    return (state == 4);
}

/*=========================================================
    接続待機
=========================================================*/

int Network_WaitConnection(void)
{
    while(!Network_IsConnected())
    {
        sceKernelDelayThread(100000);
    }

    return 0;
}

/*=========================================================
    IP取得
=========================================================*/

int Network_GetIP(char *ip)
{
    union SceNetApctlInfo info;

    memset(&info, 0, sizeof(info));

    if(sceNetApctlGetInfo(
        PSP_NET_APCTL_INFO_IP,
        &info) < 0)
    {
        return -1;
    }

    strcpy(ip, info.ip);

    return 0;
}

/*=========================================================
    IP表示
=========================================================*/

void Network_PrintIP(void)
{
    char ip[16];

    if(Network_GetIP(ip) < 0)
    {
        pspDebugScreenPrintf("IP : Unknown\n");
        return;
    }

    pspDebugScreenPrintf("IP : %s\n", ip);
}
