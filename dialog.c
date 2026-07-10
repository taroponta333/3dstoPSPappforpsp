#include <pspkernel.h>
#include <pspdisplay.h>

#include <psputility.h>
#include <psputility_netconf.h>
#include <psputility_modules.h>
#include <psputility_sysparam.h>

#include <string.h>

#include "dialog.h"

int Dialog_ShowNetwork(void)
{
    int status;

    pspUtilityNetconfData dialog;

    memset(&dialog, 0, sizeof(dialog));

    /* ネットワークモジュール読込 */

    sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    sceUtilityLoadNetModule(PSP_NET_MODULE_INET);

    /* 共通設定 */

    dialog.base.size = sizeof(dialog);

    dialog.base.language = PSP_SYSTEMPARAM_LANGUAGE_JAPANESE;

    dialog.base.buttonSwap = PSP_UTILITY_ACCEPT_CROSS;

    dialog.base.graphicsThread = 17;

    dialog.base.accessThread = 19;

    dialog.base.fontThread = 18;

    dialog.base.soundThread = 16;

    dialog.action = PSP_NETCONF_ACTION_CONNECTAP;

    dialog.hotspot = 1;

    dialog.hotspot_connected = 0;

    dialog.wifisp = 1;

    /* ダイアログ開始 */

    if (sceUtilityNetconfInitStart(&dialog) < 0)
    {
        sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
        sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);

        return -1;
    }

    while (1)
    {
        status = sceUtilityNetconfGetStatus();

        switch (status)
        {
            case PSP_UTILITY_DIALOG_INIT:

                break;

            case PSP_UTILITY_DIALOG_VISIBLE:

                sceUtilityNetconfUpdate(1);

                break;

            case PSP_UTILITY_DIALOG_QUIT:

                sceUtilityNetconfShutdownStart();

                break;

            case PSP_UTILITY_DIALOG_FINISHED:

                sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
                sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);

                return 0;

            case PSP_UTILITY_DIALOG_NONE:

                sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
                sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);

                return 0;

            default:

                break;
        }

        sceDisplayWaitVblankStart();
    }

    return 0;
}
