#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

#include "dialog.h"
#include "network.h"

PSP_MODULE_INFO("3DS App Receiver", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/*=========================================================
    Exit Callback
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

    if(cbid >= 0)
    {
        sceKernelRegisterExitCallback(cbid);
    }

    sceKernelSleepThreadCB();

    return 0;
}

int SetupCallbacks(void)
{
    int thid;

    thid = sceKernelCreateThread(
        "exit_thread",
        CallbackThread,
        0x11,
        0xFA0,
        PSP_THREAD_ATTR_USER,
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

    /* Initialize display */
    sceDisplaySetMode(0, 480, 272);
    sceDisplaySetFrameBuf(sceGeEdramGetAddr(), 512, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_ROTATE_0);

    pspDebugScreenInit();
    pspDebugScreenSetBackColor(0x00000000);
    pspDebugScreenSetTextColor(0xFFFFFFFF);
    pspDebugScreenClear();

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
        pspDebugScreenPrintf("\nPress HOME to exit.\n");

        sceKernelSleepThread();
        return 1;
    }

    pspDebugScreenPrintf("[OK] Network Initialized\n\n");

    pspDebugScreenPrintf("Waiting for connection dialog...\n");
    pspDebugScreenPrintf("(Select your WiFi network)\n\n");

    ret = Dialog_ShowNetwork();

    if(ret < 0)
    {
        pspDebugScreenPrintf("\nNetwork Dialog Failed!\n");
        pspDebugScreenPrintf("Error : 0x%08X\n", ret);
        pspDebugScreenPrintf("\nPress HOME to exit.\n");

        sceKernelSleepThread();
        return 1;
    }

    pspDebugScreenClear();

    pspDebugScreenPrintf("=================================\n");
    pspDebugScreenPrintf("      3DS App Receiver\n");
    pspDebugScreenPrintf("=================================\n\n");

    pspDebugScreenPrintf("Connected!\n\n");

    Network_PrintIP();

    pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("-------------------------------\n");
    pspDebugScreenPrintf("Waiting for 3DS...\n");
    pspDebugScreenPrintf("-------------------------------\n\n");

    pspDebugScreenPrintf("Press HOME to exit.\n");

    /* Wait for exit via callback */
    sceKernelSleepThread();

    Network_Shutdown();

    return 0;
}
