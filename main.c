#include <pspkernel.h>
#include <pspdebug.h>
#include <psputility.h>
#include <psputility_netconf.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdio.h>
PSP_MODULE_INFO("3DS_App_Receiver",0,1,0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);
int main(void){
 pspDebugScreenInit();
 printf("3DS App Receiver\n");
 printf("Skeleton source generated.\n");
 sceKernelExitGame();
 return 0;
}
