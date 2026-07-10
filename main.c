#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

#include "dialog.h"
#include "network.h"

PSP_MODULE_INFO("3DS App Receiver", PSP_MODULE_USER, 1, 0);
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
        0x20,
        0xFA0,
        0,
        NULL);

    if(thid >= 0)
        sceKernelStartThread(thid, 0, NULL);

    return thid;
}

/*=========================================================
    Main
=========================================================*/

int main(void)
{
    int ret;

    SetupCallbacks();

    pspDebugScreenInit();

    pspDebugScreenPrintf("=================================\n");
    pspDebugScreenPrintf("      3DS App Receiver\n");
    pspDebugScreenPrintf("             v0.5\n");
    pspDebugScreenPrintf("=================================\n\n");

    pspDebugScreenPrintf("Initializing Network...\n");

    ret = Network_Init();

    if(ret < 0)
    {
        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("Network Init Failed!\n");
        pspDebugScreenPrintf("Error : 0x%08X\n", ret);

        while(1)
            sceDisplayWaitVblankStart();
    }

    pspDebugScreenPrintf("[OK] Network Initialized\n");

    pspDebugScreenPrintf("\n");

    /* 接続ダイアログ */

    ret = Dialog_ShowNetwork();

    if(ret < 0)
    {
        pspDebugScreenPrintf("Network Dialog Failed!\n");
        pspDebugScreenPrintf("Error : 0x%08X\n", ret);

        while(1)
            sceDisplayWaitVblankStart();
    }

    pspDebugScreenPrintf("Connected!\n");

    pspDebugScreenPrintf("\n");

    Network_PrintIP();

    pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("-------------------------------\n");
    pspDebugScreenPrintf("Waiting for 3DS...\n");
    pspDebugScreenPrintf("-------------------------------\n\n");

    pspDebugScreenPrintf("HOME : Exit\n");

    while(1)
    {
        sceDisplayWaitVblankStart();

        /*

        v0.6
            Receiver_Update();

        v0.7
            File_Update();

        */

    }

    return 0;
}
