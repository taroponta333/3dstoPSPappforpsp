#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>

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
        sceKernelStartThread(thid, 0, NULL);

    return thid;
}

/*=========================================================
    Main
=========================================================*/

int main(void)
{
    SetupCallbacks();

    pspDebugScreenInit();

    pspDebugScreenPrintf("=================================\n");
    pspDebugScreenPrintf("      3DS App Receiver\n");
    pspDebugScreenPrintf("            v0.5.1\n");
    pspDebugScreenPrintf("=================================\n\n");

    pspDebugScreenPrintf("Step 1 : Main Only\n");
    pspDebugScreenPrintf("PSP Boot Success!\n\n");

    pspDebugScreenPrintf("Press HOME to exit.\n");

    while (1)
    {
        sceDisplayWaitVblankStart();
    }

    return 0;
}
