#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <sstream>

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

struct HttpRequest {
    string method;
    string path;
    string version;
};

bool parseHTTP(const string& request, HttpRequest& out) {
    istringstream iss(request);
    if (!(iss >> out.method >> out.path >> out.version)) {
        return false;
    }
    return true;
}

bool recvHTTP(SOCKET s, string& request) {
    char buffer[4096];
    while (true) {
        int n = recv(s, buffer, sizeof(buffer), 0);
        if (n <= 0)
            return false;
        request.append(buffer, n);
        if (request.find("\r\n\r\n") != string::npos)
            return true;
    }
}

bool sendAll(SOCKET s, const char* data, int size) {
    int total = 0;
    while (total < size) {
        int sent = send(s, data + total, size - total, 0);
        if (sent <= 0) return false;
        total += sent;
    }
    return true;
}

DWORD WINAPI ClientHandler(LPVOID lpParam) {
    SOCKET ClientSocket = (SOCKET)lpParam;
    cout << "Клиент подключился\n";

    int timeout = 10000;
    setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(ClientSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    string request;
    if (!recvHTTP(ClientSocket, request)) {
        cout << "Ошибка HTTP чтения\n";
    }
    cout << "Получен запрос:\n" << request << "\n";

    // формирование ответа
    string response;
    string html;
    HttpRequest req;
    if (!parseHTTP(request, req)) {
        cout << "Ошибка парсинга HTTP\n";
        closesocket(ClientSocket);
        return 0;
    }
    if (req.method == "GET") {
        html =
            "<html><body><h1>Hello from HTTP Server</h1></body></html>";
        response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: " + to_string(html.size()) + "\r\n"
            "Connection: close\r\n\r\n" +
            html;
    }
    else {
        html =
            "<html><body><h1>400 Bad Request</h1></body></html>";
        response =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: " + to_string(html.size()) + "\r\n"
            "Connection: close\r\n\r\n" +
            html;
    }

    if (!sendAll(ClientSocket, response.c_str(), (int)response.size())) {
        cout << "Ошибка отправки HTTP-ответа\n\n";
        closesocket(ClientSocket);
        return 0;
    }

    cout << "Ответ отправлен клиенту\n\n";

    closesocket(ClientSocket);
    return 0;
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    WSADATA wsData; // включает работу с сетью
    int erStat = WSAStartup(MAKEWORD(2, 2), &wsData);
    if (erStat != 0) {
        cout << "Ошибка инициализации winsock: " << WSAGetLastError() << "\n";
        system("pause");
        return 1;
    }

    SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == INVALID_SOCKET) {
        cout << "Ошибка инициализации сокета: " << WSAGetLastError() << "\n";
        system("pause");
        WSACleanup();
        return 1;
    }

    sockaddr_in servInfo;
    ZeroMemory(&servInfo, sizeof(servInfo));
    servInfo.sin_family = AF_INET;
    servInfo.sin_addr.s_addr = INADDR_ANY;
    servInfo.sin_port = htons(PORT);
    erStat = bind(ServerSocket, (sockaddr*)&servInfo, sizeof(servInfo));
    if (erStat != 0) {
        cout << "Ошибка привязки сервера к порту (bind): " << WSAGetLastError() << "\n";
        system("pause");
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }

    erStat = listen(ServerSocket, SOMAXCONN); // somaxconn - максимальное число подключений
    // либо, если надо N подключений, SOMAXCONN_HINT(N)
    if (erStat != 0) {
        cout << "Ошибка прослушивания подключений: " << WSAGetLastError() << "\n";
        system("pause");
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }

    cout << "HTTP-сервер запущен (порт " << PORT << "). Ожидание подключений...\n";

    while (true) {
        SOCKET ClientSocket = accept(ServerSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            cout << "Обнаружен клиент. Ошибка соединения: " << WSAGetLastError() << "\n";
            continue;
        }
        HANDLE hThread = CreateThread(NULL, 0, ClientHandler, (LPVOID)ClientSocket, 0, NULL);
        CloseHandle(hThread);
    }

    closesocket(ServerSocket);
    WSACleanup();
    return 0;
}