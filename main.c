#include <psputility_netconf.h> // 💡 全ての元凶はこれの不在でした！これを入れることで100%解決します。
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
#include <pspctrl.h>
#include <pspdisplay.h>

PSP_MODULE_INFO("3DS_Receiver", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define RECV_PORT 8080
#define PACKET_SIZE 1448
#define SAVE_PATH "ms0:/received_file.pbp"

int exit_request = 0;

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
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

// ✨ 改良：ヘッダを追加したことで、公式通りの一番綺麗な記述が使えるようになりました
int show_network_connect_dialog(void) {
    pspUtilityNetconfData data;
    memset(&data, 0, sizeof(data));
    
    data.base.size = sizeof(data);
    data.base.language = PSP_UTILITY_LANG_JAPANESE; 
    data.base.buttonSwap = 0;    // ○ボタン決定
    data.base.graphicsThread = 17;
    data.base.accessThread = 19;
    data.base.fontThread = 18;
    data.base.soundThread = 16;
    
    // 正式な「アクセスポイント接続」のアクションを指定
    data.action = PSP_NETCONF_ACTION_CONNECTAP; 

    int ret = sceUtilityNetconfInitStart(&data);
    if (ret < 0) return ret;

    while (1) {
        int status = sceUtilityNetconfGetStatus();
        if (status == PSP_UTILITY_DIALOG_VISIBLE) { 
            sceUtilityNetconfUpdate(1);
        } else if (status == PSP_UTILITY_DIALOG_FINISHED) { 
            sceUtilityNetconfShutdownStart();
            break;
        }
        sceDisplayWaitVblankStart();
    }

    // 正式な「IPアドレス取得完了」の定数でチェック
    int ap_status;
    if (sceNetApctlGetState(&ap_status) == 0 && ap_status == PSP_NETAPCTL_STATE_GOTIP) {
        return 0; // 接続成功
    }
    return -1;
}

// 安全に終了メッセージを出して終了する関数（電源落ち防止）
void safe_exit(const char *error_msg) {
    pspDebugScreenClear();
    printf("==========================================\n");
    printf("       PSP Wireless File Receiver         \n");
    printf("==========================================\n\n");
    if (error_msg) {
        printf("[ERR] %s\n", error_msg);
    }
    printf("\nExiting application in 5 seconds...\n");
    sceKernelDelayThread(5000000); 
    sceKernelExitGame();
}

int show_exit_dialog(void) {
    pspUtilityMsgDialogParams dialog;
    memset(&dialog, 0, sizeof(dialog));
    
    dialog.base.size = sizeof(dialog);
    dialog.base.language = PSP_UTILITY_LANG_JAPANESE;
    dialog.base.buttonSwap = 0;
    dialog.base.graphicsThread = 17;
    dialog.base.accessThread = 19;
    dialog.base.fontThread = 18;
    dialog.base.soundThread = 16;
    
    dialog.mode = PSP_UTILITY_MSGDIALOG_MODE_TEXT;
    dialog.options = PSP_UTILITY_MSGDIALOG_OPTION_TEXT; 
    snprintf(dialog.message, 512, "Do you want to quit the application?");

    sceUtilityMsgDialogInitStart(&dialog);

    while (1) {
        int status = sceUtilityMsgDialogGetStatus();
        if (status == PSP_UTILITY_DIALOG_VISIBLE) { 
            sceUtilityMsgDialogUpdate(1);
        } else if (status == PSP_UTILITY_DIALOG_FINISHED) { 
            if (dialog.buttonPressed == PSP_UTILITY_MSGDIALOG_RESULT_YES) { 
                sceUtilityMsgDialogShutdownStart();
                return 1;
            }
            sceUtilityMsgDialogShutdownStart();
            return 0;
        }
        sceDisplayWaitVblankStart(); 
    }
}

int init_network(void) {
    if (sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON) < 0) return -1;
    if (sceUtilityLoadNetModule(PSP_NET_MODULE_INET) < 0) return -1;
    if (sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024) < 0) return -1;
    if (sceNetInetInit() < 0) return -1;
    if (sceNetResolverInit() < 0) return -1;
    if (sceNetApctlInit(0x1400, 42) < 0) return -1;
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

void print_status_line(int y, const char *msg) {
    pspDebugScreenSetXY(0, y);
    printf("                                                                ");
    pspDebugScreenSetXY(0, y);
    printf("%s", msg);
}

int main(void) {
    pspDebugScreenInit();
    setup_callbacks();
    
    printf("==========================================\n");
    printf("       PSP Wireless File Receiver         \n");
    printf("==========================================\n\n");
    
    printf("[SYS] Initializing network modules...\n");
    if (init_network() < 0) {
        safe_exit("Network initialization failed!");
        return 0;
    }

    // Wi-Fi接続画面を表示（WPA2プロファイル選択用）
    printf("[SYS] Opening Network Connection Dialog...\n");
    if (show_network_connect_dialog() < 0) {
        shutdown_network();
        safe_exit("Failed to connect to Wi-Fi. (Canceled or Timeout)");
        return 0;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        shutdown_network();
        safe_exit("Socket creation failed!");
        return 0;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(RECV_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        shutdown_network();
        safe_exit("Socket bind failed!");
        return 0;
    }

    struct timeval timeout = {0, 100000}; // タイムアウト0.1秒
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char buffer[PACKET_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    FILE *file = NULL;
    
    int is_transferring = 0;
    unsigned int total_file_size = 0;
    unsigned int received_bytes_sum = 0;

    SceCtrlData pad;

    const int STATUS_LINE_Y = 8;
    const int PROGRESS_LINE_Y = 10;

    // 現在のIPアドレスを取得して表示
    char my_ip[32] = "Unknown";
    sceNetApctlGetInfo(8, my_ip); 
    
    pspDebugScreenSetXY(0, 5);
    printf("[SYS] Wi-Fi Connected! IP: %s\n", my_ip);

    print_status_line(STATUS_LINE_Y, "[STATUS] Ready. Waiting for 3DS broadcast...");
    pspDebugScreenSetXY(0, STATUS_LINE_Y + 4);
    printf("(Press TRIANGLE to quit)\n");

    while (!exit_request) {
        sceCtrlPeekBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_TRIANGLE) { 
            if (show_exit_dialog()) { 
                break; 
            } else {
                pspDebugScreenClear();
                printf("==========================================\n");
                printf("       PSP Wireless File Receiver         \n");
                printf("==========================================\n\n");
                printf("[SYS] Wi-Fi Connected! IP: %s\n", my_ip);
                if (is_transferring) {
                    print_status_line(STATUS_LINE_Y, "[STATUS] Downloading data from 3DS...");
                } else {
                    print_status_line(STATUS_LINE_Y, "[STATUS] Ready. Waiting for 3DS broadcast...");
                }
                pspDebugScreenSetXY(0, STATUS_LINE_Y + 4);
                printf("(Press TRIANGLE to quit)\n");
            }
        }

        int bytes_received = recvfrom(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (bytes_received > 0) { 
            if (!is_transferring) { 
                file = fopen(SAVE_PATH, "wb"); 
                is_transferring = 1; 
                print_status_line(STATUS_LINE_Y, "[STATUS] Connection established! Receiving data...");
                
                if (bytes_received >= sizeof(unsigned int)) {
                    memcpy(&total_file_size, buffer, sizeof(unsigned int));
                    int data_size = bytes_received - sizeof(unsigned int);
                    if (data_size > 0 && file) {
                        fwrite(buffer + sizeof(unsigned int), 1, data_size, file);
                        received_bytes_sum += data_size;
                    }
                }
            } else {
                if (file) { 
                    fwrite(buffer, 1, bytes_received, file); 
                    received_bytes_sum += bytes_received;
                } 
            }

            char progress_msg[128];
            if (total_file_size > 0) {
                int percent = (int)(((long long)received_bytes_sum * 100) / total_file_size);
                if (percent > 100) percent = 100;
                snprintf(progress_msg, sizeof(progress_msg), "Progress: %d%% (%u / %u bytes)", percent, received_bytes_sum, total_file_size);
            } else {
                snprintf(progress_msg, sizeof(progress_msg), "Progress: ---%% (%u bytes received)", received_bytes_sum);
            }
            print_status_line(PROGRESS_LINE_Y, progress_msg);

        } else if (is_transferring) { 
            if (file) fclose(file); 
            file = NULL;
            print_status_line(STATUS_LINE_Y, "[STATUS] Transfer complete! File saved successfully.");
            sceKernelDelayThread(3000000); 
            break; 
        }
    }

    if (file) fclose(file); 
    close(sock); 
    shutdown_network(); 
    sceKernelExitGame(); 
    return 0; 
}
