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
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

PSP_MODULE_INFO("3DS_Receiver", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define RECV_PORT 8080
#define PACKET_SIZE 1448
#define SAVE_PATH "ms0:/received_file.pbp"

// HOMEボタンで安全に終了するためのコールバック設定
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

// PSPのネットワーク機能を呼び出す関数（✨ 最新SDKのルールに合わせて引数を追加）
int init_network(void) {
    sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
    
    // 引数：プールサイズ(128KB), コールアウト優先度(42), スタック(4KB), 割り込み優先度(42), スタック(4KB)
    sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024);
    sceNetInetInit();
    sceNetResolverInit();
    
    // 引数：スタックサイズ(約5KB), 優先度(42)
    sceNetApctlInit(0x1400, 42);
    return 0;
}

// ネットワークの後片付け
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
    printf("Initializing network...\n");
    
    if (init_network() < 0) {
        printf("Network init failed!\n");
        sceKernelDelayThread(3000000);
        sceKernelExitGame();
        return 0;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("Socket creation failed!\n");
        goto cleanup;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(RECV_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind failed! Port 8080 might be in use.\n");
        close(sock);
        goto cleanup;
    }

    struct timeval timeout;
    timeout.tv_sec = 3; 
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    printf("\nReady! Waiting for 3DS broadcast on Port %d...\n", RECV_PORT);
    printf("Please press 'A' on your 3DS app.\n\n");

    char buffer[PACKET_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    FILE *file = NULL;
    int is_transferring = 0;
    int total_packets = 0;

    while (!exit_request) {
        int bytes_received = recvfrom(sock, buffer, PACKET_SIZE, 0, 
                                      (struct sockaddr *)&client_addr, &addr_len);

        if (bytes_received > 0) {
            if (!is_transferring) {
                printf("Connection detected! Receiving file...\n");
                file = fopen(SAVE_PATH, "wb");
                if (file == NULL) {
                    printf("Error: Cannot create file on Memory Stick!\n");
                    break;
                }
                is_transferring = 1;
            }

            fwrite(buffer, 1, bytes_received, file);
            total_packets++;

            if (total_packets % 50 == 0) {
                printf(".");
            }
        } else {
            if (is_transferring) {
                printf("\n\nTransfer Complete successfully!\n");
                printf("Saved to: %s\n", SAVE_PATH);
                printf("Total Packets Received: %d\n", total_packets);
                fclose(file);
                file = NULL;
                break;
            }
        }
    }

    if (file) fclose(file);
    close(sock);

cleanup:
    printf("\nExiting in 5 seconds...\n");
    shutdown_network();
    sceKernelDelayThread(5000000);
    sceKernelExitGame();
    return 0;
}
