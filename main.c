#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>

PSP_MODULE_INFO("HelloPSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* -------- HOMEボタン対応 -------- */

int exit_callback(int arg1, int arg2, void *common)
{
    sceKernelExitGame();
    return 0;
}

int CallbackThread(SceSize args, void *argp)
{
    int cbid;

    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);

    sceKernelSleepThreadCB();

    return 0;
}

int SetupCallbacks(void)
{
    int thid = sceKernelCreateThread(
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

/* ------------------------------ */

int main(void)
{
    SetupCallbacks();

    pspDebugScreenInit();

    pspDebugScreenPrintf("=================================\n");
    pspDebugScreenPrintf("        3DS App Receiver\n");
    pspDebugScreenPrintf("=================================\n\n");

    pspDebugScreenPrintf("Hello PSP!\n");
    pspDebugScreenPrintf("This program is running.\n\n");
    pspDebugScreenPrintf("Press HOME to exit.\n");

    while (1)
    {
        sceDisplayWaitVblankStart();
    }

    return 0;
}
