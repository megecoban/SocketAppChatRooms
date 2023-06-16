t#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <stdexcept>

using namespace std;

int SunucuPortu = 7778;

bool baglandiMi = false;

class Istemci 
{
    public:
        int sunucuPort = SunucuPortu;
        int istemciSoketi;
        int bufferBoyutu = 1024;
        struct sockaddr_in istemciAddr;
        socklen_t istemciAddrLen = sizeof(istemciAddr);
};

void SocketBaslat();

void ClientBaslat(){
    SocketBaslat();
}

void* SunucuyaMesajGonder(void* args)
{
    int *istemciSoketi = (int*)args;
    char msg[1024] = {0};
    string smsg;
    while (1 == 1)
    {
        if(baglandiMi==false)
            break;

        memset(msg, 0, sizeof(msg));
        cout << "Mesaj: \n";
        getline(cin, smsg);
        strncpy(msg, smsg.c_str(), sizeof(msg) - 1);
        send(*istemciSoketi, msg, 1024, 0);
    }

    pthread_exit(NULL);
}

void* SunucuKontrol(void* args)
{
    int clientSocket = *((int*)args);
    char buffer[1024];

    while(1==1)
    {
        memset(buffer, 0, sizeof(buffer));
        ssize_t readVal = recv(clientSocket, buffer, sizeof(buffer) - 1, MSG_PEEK);

        if(readVal==0)
        {
            baglandiMi = false;
            break;
        }
    }
    cout << "Sunucuya baglanilamadi. Sunucu dolu olabilir." <<endl;
    close(clientSocket);
    pthread_exit(NULL);
}

void* SunucudanOku(void* args)
{
    int clientSocket = *((int*)args);
    char buffer[1024] = {0};

    while(1==1)
    {
        memset(buffer, 0, sizeof(buffer));
        int readVal = read(clientSocket, buffer, 1024);

        if(readVal>0)
        {
            cout << buffer <<endl;
        }
    }

    close(clientSocket);
    pthread_exit(NULL);
}

void SocketBaslat(){
    Istemci istemci;

    const char ServerIP[] = "127.0.0.1";

    if((istemci.istemciSoketi = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\nSoket acilamadi.");
        exit(EXIT_FAILURE);
    }else{
        cout<<"\nSoket acildi."<<endl;
    }

    istemci.istemciAddr.sin_family = AF_INET;
    istemci.istemciAddr.sin_port = htons(istemci.sunucuPort);

    if(inet_pton(AF_INET, ServerIP, &istemci.istemciAddr.sin_addr) <= 0){
        perror("\nGecersiz IP adresi");
        exit(EXIT_FAILURE);
    }

    int connectionSock;
    while (true)
    {
        connectionSock = connect(
            istemci.istemciSoketi,
            (struct sockaddr*)&istemci.istemciAddr,
            sizeof(istemci.istemciAddr)
        );
        if(connectionSock > -1) break;
        printf("Sunucu araniyor...\n");
        sleep(3);
    }

    baglandiMi = true;

    pthread_t sunucuKontrol;
    pthread_t sunucudanOku;
    pthread_t sunucuyaYaz;
    pthread_create(&sunucuKontrol, NULL, SunucuKontrol, &istemci.istemciSoketi);
    pthread_create(&sunucudanOku, NULL, SunucudanOku, &istemci.istemciSoketi);
    pthread_create(&sunucuyaYaz, NULL, SunucuyaMesajGonder, &istemci.istemciSoketi);
    pthread_join(sunucuKontrol, NULL);
    pthread_join(sunucudanOku, NULL);
    pthread_join(sunucuyaYaz, NULL);
}

int main(){
    ClientBaslat();
    return 0;
}
