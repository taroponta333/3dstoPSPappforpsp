#include <pspkernel.h>
#include <pspdebug.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
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

// 🌐 ネットワークモジュールの最小限の初期化
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
        printf("[ERR] Network initialization failed!\n");
        sceKernelDelayThread(3000000);
        sceKernelExitGame();
        return 0;
    }

    // 🌐 【大修正】公式の接続画面を出さず、本体の「接続設定1番」で自動接続
    printf("[SYS] Connecting to Wi-Fi (Connection Profile 1)...\n");
    if (sceNetApctlConnect(1) < 0) {
        shutdown_network();
        printf("[ERR] Failed to start connection.\n");
        sceKernelDelayThread(3000000);
        sceKernelExitGame();
        return 0;
    }

    // 接続完了（IPアドレス取得状態 = 4）になるまで画面を止めずに待つ
    int state;
    while (1) {
        if (sceNetApctlGetState(&state) == 0) {
            if (state == 4) { // 4 = STATE_GOTIP (SDKのバージョン違いを避けるため数値で直接指定)
                break;
            }
        }
        sceKernelDelayThread(50000); // 50msごとにチェック
    }
    printf("[SYS] Wi-Fi Connected successfully!\n\n");

    // 📡 ソケット作成
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        shutdown_network();
        printf("[ERR] Socket creation failed!\n");
        sceKernelDelayThread(3000000);
        sceKernelExitGame();
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
        printf("[ERR] Socket bind failed!\n");
        sceKernelDelayThread(3000000);
        sceKernelExitGame();
        return 0;
    }

    // 0.1秒の受信タイムアウト設定
    struct timeval timeout = {0, 100000}; 
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char buffer[PACKET_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    FILE *file = NULL;
    
    int is_transferring = 0;
    unsigned int total_file_size = 0;
    unsigned int received_bytes_sum = 0;
    SceCtrlData pad;

    const int STATUS_LINE_Y = 9;
    const int PROGRESS_LINE_Y = 11;

    print_status_line(STATUS_LINE_Y, "[STATUS] Ready. Waiting for 3DS broadcast...");
    pspDebugScreenSetXY(0, STATUS_LINE_Y + 4);
    printf("(Press TRIANGLE to quit)\n");

    while (!exit_request) {
        sceCtrlPeekBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_TRIANGLE) { 
            // 🛑 【大修正】ダイアログを出さず即終了
            break; 
        }

        int bytes_received = recvfrom(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (bytes_received > 0) { 
            if (!is_transferring) { 
                file = fopen(SAVE_PATH, "wb"); 
                if (!file) {
                    print_status_line(STATUS_LINE_Y, "[STATUS] Error: Cannot open file for writing.");
                    break;
                }
                is_transferring = 1; 
                print_status_line(STATUS_LINE_Y, "[STATUS] Connection established! Receiving data...");
                
                if (bytes_received >= sizeof(unsigned int)) {
                    memcpy(&total_file_size, buffer, sizeof(unsigned int));
                    int data_size = bytes_received - sizeof(unsigned int);
                    if (data_size > 0) {
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

            // データが指定サイズまで届ききったら正常終了
            if (total_file_size > 0 && received_bytes_sum >= total_file_size) {
                if (file) fclose(file); 
                file = NULL;
                print_status_line(STATUS_LINE_Y, "[STATUS] Transfer complete! File saved successfully.");
                sceKernelDelayThread(3000000); 
                break; 
            }
        }
    }

    if (file) fclose(file); 
    close(sock); 
    shutdown_network(); 
    sceKernelExitGame(); 
    return 0; 
}
