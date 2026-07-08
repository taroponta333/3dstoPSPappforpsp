#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>

PSP_MODULE_INFO("HelloPSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int main(void)
{
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
