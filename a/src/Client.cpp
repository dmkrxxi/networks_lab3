#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int PORT = 12345;
const int MAX_SIZE = 4096;

/*bool recvAll(SOCKET s, char* data, int size) {
    int total = 0;
    while (total < size) {
        int received = recv(s, data + total, size - total, 0);
        if (received <= 0) return false;
        total += received;
    }
    return true;
}*/

bool sendAll(SOCKET s, const char* data, int size) {
    int total = 0;
    while (total < size) {
        int sent = send(s, data + total, size - total, 0);
        if (sent <= 0) return false;
        total += sent;
    }
    return true;
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    WSADATA wsData;
    int erStat = WSAStartup(MAKEWORD(2, 2), &wsData);
    if (erStat != 0) {
        cout << "Ошибка инициализации winsock: " << WSAGetLastError() << "\n";
        system("pause");
        return 1;
    }

    SOCKET ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "Ошибка инициализации сокета: " << WSAGetLastError() << "\n";
        system("pause");
        WSACleanup();
        return 1;
    }

    // 10 с - ограничиваем время для операций приема и отправки
    int timeout = 10000;
    setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(ClientSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    sockaddr_in servInfo;
    ZeroMemory(&servInfo, sizeof(servInfo));
    servInfo.sin_family = AF_INET;
    servInfo.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &servInfo.sin_addr) <= 0) {
        cout << "Ошибка преобразования адреса\n";
        system("pause");
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    erStat = connect(ClientSocket, (sockaddr*)&servInfo, sizeof(servInfo));
    if (erStat != 0) {
        cout << "Ошибка подключения к серверу: " << WSAGetLastError() << "\n";
        system("pause");
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    string request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n\r\n";

    if (!sendAll(ClientSocket, request.c_str(), (int)request.size())) {
        cout << "Ошибка отправки запроса\n";
        system("pause");
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    char buffer[MAX_SIZE];
    int recvBytes;

    cout << "Ответ сервера:\n";

    while ((recvBytes = recv(ClientSocket, buffer, MAX_SIZE - 1, 0)) > 0) {
        buffer[recvBytes] = '\0';
        cout << buffer;
    }

    if (recvBytes < 0) {
        cout << "Ошибка получения ответа\n";
    }

    closesocket(ClientSocket);
    WSACleanup();

    cout << "\nЗавершение работы клиента.\n";
    system("pause");
    return 0;
}