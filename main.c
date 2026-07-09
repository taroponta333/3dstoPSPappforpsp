#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>

#include <pspnet.h>
#include <pspnet_apctl.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>

#include <psputility.h>
#include <psputility_netconf.h>

#include <stdio.h>
#include <string.h>
PSP_MODULE_INFO("3DS App Receiver", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);





オファーを受け取る


3dstoPSPappforpsp-main (1).zip
Zip アーカイブ
Deep Research
 githubでeboot.pbpをビルドしようと思ったけどエラーがエグい



なんか設定のとこでエラー吐いてる気がする


3dstoPSPappforpsp-main (2).zip
Zip アーカイブ
https://github.com/taroponta333/3dstoPSPappforpsp


main (1).c
ファイル

PSPのオンライン機能復活について.pdf
PDF
あといままでこれを何故作ろうとしたのかの話


psp-to-3ds-app-main (1).zip
Zip アーカイブ
じゃあ一旦3DS側のコード（生成ができることは確認済み）出すからそれに合わせて書いてほしい


自分のpsp改造歴史.pdf
PDF
バッテリーの話は伏線でした
そして時が流れて小6この頃AIがめっちゃ高性能に
そこで一旦バッテリー回復を目指して2時間くらい充電
つけてみるとあれ？なんでオートブートなの？もしかしてパンドラ？
その後mｍs作成しようにもクラシックパンドラのみ
入れてみると海外版ぷよぷよで600に！！
あとはwebアーカイブで661のデータ取得
pro導入
ark-4を知る
導入

6時以降らしい

おｋ！

よし！じゃあ v0.3 Part 1 いこう！🚀

今回はコンパイルが通ることを最優先にして、HOME終了とネットワーク初期化まで実装するよ。

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
int InitNetwork(void)
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
int IsConnected(void)
{
    int state;

    if(sceNetApctlGetState(&state) < 0)
        return 0;

    return (state == 4);
}
void PrintConnectionState(void)
{
    int state;

    sceNetApctlGetState(&state);

    pspDebugScreenPrintf("\n");

    switch(state)
    {
        case 0:
            pspDebugScreenPrintf("Disconnected\n");
            break;

        case 1:
            pspDebugScreenPrintf("Scanning...\n");
            break;

        case 2:
            pspDebugScreenPrintf("Connecting...\n");
            break;

        case 3:
            pspDebugScreenPrintf("Getting IP...\n");
            break;

        case 4:
            pspDebugScreenPrintf("Connected!\n");
            break;

        default:
            pspDebugScreenPrintf("Unknown State : %d\n", state);
            break;
    }
}
void PrepareNetDialog(void)
{
    pspUtilityNetconfData dialog;

    memset(&dialog, 0, sizeof(dialog));

    dialog.base.size = sizeof(dialog);

    dialog.action = PSP_NETCONF_ACTION_CONNECTAP;
}
/*=========================================================
    メイン
=========================================================*/

int main(void)
{
    int ret;

    SetupCallbacks();

    pspDebugScreenInit();

    pspDebugScreenPrintf("=================================\n");
    pspDebugScreenPrintf("      3DS App Receiver v0.4\n");
    pspDebugScreenPrintf("=================================\n\n");

    /* ネットワーク初期化 */

    ret = InitNetwork();

    if(ret < 0)
    {
        pspDebugScreenPrintf("Network Init Failed!\n");
        pspDebugScreenPrintf("Error : 0x%08X\n", ret);

        while(1)
            sceDisplayWaitVblankStart();
    }

    pspDebugScreenPrintf("[OK] Network Initialized\n");

    /* 接続状態表示 */

    PrintConnectionState();

    if(!IsConnected())
    {
        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("---------------------------------\n");
        pspDebugScreenPrintf("Wi-Fi is not connected.\n");
        pspDebugScreenPrintf("Network Dialog will be\n");
        pspDebugScreenPrintf("implemented in v0.5\n");
        pspDebugScreenPrintf("---------------------------------\n");
    }
    else
    {
        pspDebugScreenPrintf("\n");
        pspDebugScreenPrintf("---------------------------------\n");
        pspDebugScreenPrintf("Connected!\n");
        pspDebugScreenPrintf("---------------------------------\n");
    }

    pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Waiting for 3DS...\n");
    pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("HOME : Exit\n");

    while(1)
    {
        sceDisplayWaitVblankStart();

        /* v0.5
           Network Dialog

           v0.6
           Socket

           v0.7
           Receive

           をここへ追加
        */
    }

    return 0;
}
