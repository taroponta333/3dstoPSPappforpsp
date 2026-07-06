#include <pspkernel.h>
#include <pspdebug.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <psputility.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pspctrl.h> // ✨ ボタン入力を監視するために追加

// モジュール情報をカーネル寄りに変更して安定化
PSP_MODULE_INFO("3DS_Receiver", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define RECV_PORT 8080
#define PACKET_SIZE 1448
#define SAVE_PATH "ms0:/received_file.pbp"

int exit_request = 0;

// 標準の終了コールバック（HOMEメニュー用、ダイアログなしで即終了する安全版）
int exit_callback(int arg1, int arg2, void *common) {
    exit_request = 1;
    return 0;
}
int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}
void setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread=, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

// アプリ内で「本当に終了しますか？」を出す安全な関数
int show_exit_dialog(void) {
    pspUtilityMsgDialogParams dialog;
    memset(&dialog, 0, sizeof(dialog));
    
    dialog.base.size = sizeof(dialog);
    dialog.base.language = 1;
    dialog.base.buttonSwap = 0;
    dialog.base.graphicsThread = 17;
    dialog.base.accessThread = 19;
    dialog.base.fontThread = 18;
    dialog.base.soundThread = 16;
    
    dialog.mode = PSP_UTILITY_MSGDIALOG_MODE_TEXT;
    dialog.option = PSP_UTILITY_MSGDIALOG_OPTION_YESNO;
    snprintf(dialog.message, 512, "Do you want to quit the application?");

    sceUtilityMsgDialogInitStart(&dialog);

    while (1) {
        int status = sceUtilityMsgDialogGetStatus();
        if (status == PSP_UTILITY_DIALOG_STATUS_RUNNING) {
            sceUtilityMsgDialogUpdate(1);
        } else if (status == PSP_UTILITY_DIALOG_STATUS_FINISHED) {
            if (dialog.buttonPressed == PSP_UTILITY_MSGDIALOG_RESULT_YES) {
                sceUtilityMsgDialogShutdownStart();
                return 1; // 終了する
            }
            sceUtilityMsgDialogShutdownStart();
            return 0; // 戻る
        }
        // ループの暴走を防ぐために長めのウェイトを入れる
        sceDisplayWaitVblankStart(); 
    }
}

int init_network(void) {
    sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
    sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024);
    sceNetInetInit();
    sceNetResolverInit();
    sceNetApctlInit(0x1400, 42);
    return 0;
}

void shutdown_network(void) {
    sceNetApctlTerm();
    sceNetResolverTerm();
    sceNetInetTerm();
    sceNetTerm();
    sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
    sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);
}

int main(void) {
    pspDebugScreenInit();
    setup_callbacks();
    
    printf("=== PSP Wireless File Receiver ===\n");
    if (init_network() < 0) {
        sceKernelExitGame();
        return 0;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(RECV_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // 通信のタイムアウトを短くしてループを回しやすくする
    struct timeval timeout = {0, 500000}; // 0.5秒
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char buffer[PACKET_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    FILE *file = NULL;
    int is_transferring = 0;

    SceCtrlData pad;

    printf("Waiting for 3DS broadcast... (Press TRIANGLE to quit)\n");

    while (!exit_request) {
        // ✨ ボタン入力を監視（毎ループチェック）
        sceCtrlPeekBufferPositive(&pad, 1);
        if (pad.buttons & PSP_CTRL_TRIANGLE) { // △ボタンが押されたら
            if (show_exit_dialog()) {
                break; // ループを抜けて安全に終了
            } else {
                printf("Waiting for 3DS broadcast... (Press TRIANGLE to quit)\n");
            }
        }

        int bytes_received = recvfrom(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (bytes_received > 0) {
            if (!is_transferring) {
                file = fopen(SAVE_PATH, "wb");
                is_transferring = 1;
                printf("Receiving data...\n");
            }
            if (file) {
                fwrite(buffer, 1, bytes_received, file);
                printf(".");
            }
        } else if (is_transferring) {
            if (file) fclose(file);
            printf("\nTransfer complete!\n");
            break;
        }
    }

    if (file) fclose(file);
    close(sock);
    shutdown_network();
    sceKernelExitGame();
    return 0;
}
