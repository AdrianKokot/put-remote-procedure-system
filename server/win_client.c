#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    WORD WRequiredVersion;
    WSADATA WData;
    SOCKET SSocket;
    int nConnect;
    int nBytes;
    struct sockaddr_in stServerAddr;
    struct hostent* lpstServerEnt;
    char cbBuf[BUF_SIZE];

    WRequiredVersion = MAKEWORD(2,0);
    WSAStartup(WRequiredVersion, &WData);
    gethostbyname(argv[1]);
    SSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    stServerAddr.sin_family = AF_INET;
    memcpy(&stServerAddr.sin_addr.s_addr, lpstServerEnt->h_addr, lpstServerEnt->h_length);
    connect(SSocket, (struct sockaddr*)&stServerAddr, sizeof(struct sockaddr));
    nBytes = recv(SSocket, cbBuf, sizeof(cbBuf), 0);
    cbBuf[nBytes] = "\x0";
    printf("Data from SERVER [%s]:\t%s", argv[1], cbBuf);
    closesocket(SSocket);
    WSACleanup();
    return 0;
}