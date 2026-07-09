#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>

#include <pspnet.h>
#include <pspnet_apctl.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>

#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("3DS App Receiver", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/*=========================================================
    HOMEボタン対応
=========================================================*/

int exit_callback(int arg1, int arg2, void *common)
{
    sceKernelExitGame();
    return 0;
}

int CallbackThread(SceSize args, void *argp)
{
    int cbid;

    cbid = sceKernelCreateCallback(
        "Exit Callback",
        exit_callback,
        NULL);

    sceKernelRegisterExitCallback(cbid);

    sceKernelSleepThreadCB();

    return 0;
}

int SetupCallbacks(void)
{
    int thid;

    thid = sceKernelCreateThread(
        "update_thread",
        CallbackThread,
        0x11,
        0xFA0,
        0,
        NULL);

    if (thid >= 0)
        sceKernelStartThread(thid, 0, 0);

    return thid;
}
/*=========================================================
    ネットワーク初期化
=========================================================*/

int InitNetwork(void)
{
    int ret;

    pspDebugScreenPrintf("\nInitializing Network...\n\n");

    ret = sceNetInit(
        128 * 1024,
        42,
        4 * 1024,
        42,
        4 * 1024);

    if (ret < 0)
    {
        pspDebugScreenPrintf(
            "sceNetInit Error : 0x%08X\n",
            ret);
        return ret;
    }

    pspDebugScreenPrintf("[OK] sceNetInit\n");

    ret = sceNetInetInit();

    if (ret < 0)
    {
        pspDebugScreenPrintf(
            "sceNetInetInit Error : 0x%08X\n",
            ret);
        return ret;
    }

    pspDebugScreenPrintf("[OK] sceNetInetInit\n");

    ret = sceNetResolverInit();

    if (ret < 0)
    {
        pspDebugScreenPrintf(
            "sceNetResolverInit Error : 0x%08X\n",
            ret);
        return ret;
    }

    pspDebugScreenPrintf("[OK] sceNetResolverInit\n");

    ret = sceNetApctlInit(
        0x1800,
        48);

    if (ret < 0)
    {
        pspDebugScreenPrintf(
            "sceNetApctlInit Error : 0x%08X\n",
            ret);
        return ret;
    }

    pspDebugScreenPrintf("[OK] sceNetApctlInit\n");

    return 0;
}
/*=========================================================
    メイン
=========================================================*/

int main(void)
{
    int ret;
    int state = 0;

    SetupCallbacks();

    pspDebugScreenInit();

    pspDebugScreenPrintf("=================================\n");
    pspDebugScreenPrintf("      3DS App Receiver v0.3\n");
    pspDebugScreenPrintf("=================================\n\n");

    /* ネットワーク初期化 */
    ret = InitNetwork();

    if (ret < 0)
    {
        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("Network initialization failed.\n");
        pspDebugScreenPrintf("Error : 0x%08X\n", ret);
        pspDebugScreenPrintf("\nPress HOME to exit.\n");

        while (1)
        {
            sceDisplayWaitVblankStart();
        }
    }

    pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Network initialized successfully!\n");

    /* 接続状態取得 */
    sceNetApctlGetState(&state);

    pspDebugScreenPrintf("\n");

    switch(state)
    {
        case 0:
            pspDebugScreenPrintf("Wi-Fi : Disconnected\n");
            break;

        case 1:
            pspDebugScreenPrintf("Wi-Fi : Scanning...\n");
            break;

        case 2:
            pspDebugScreenPrintf("Wi-Fi : Connecting...\n");
            break;

        case 3:
            pspDebugScreenPrintf("Wi-Fi : Obtaining IP...\n");
            break;

        case 4:
            pspDebugScreenPrintf("Wi-Fi : Connected!\n");
            break;

        default:
            pspDebugScreenPrintf("Wi-Fi : Unknown (%d)\n", state);
            break;
    }

    pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("---------------------------------\n");
    pspDebugScreenPrintf("Waiting for 3DS...\n");
    pspDebugScreenPrintf("---------------------------------\n\n");

    pspDebugScreenPrintf("HOME : Exit\n");

    while (1)
    {
        sceDisplayWaitVblankStart();

        /* 接続状態を更新 */
        sceNetApctlGetState(&state);

        /* v0.4以降で
           ・接続ダイアログ
           ・UDP待受
           ・受信処理
           をここへ追加する */
    }

    return 0;
}
