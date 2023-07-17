#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <opencv2/opencv.hpp>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

void sendimage(const std::string& imagePath, SOCKET clientSocket)
{
    std::ifstream inputFile(imagePath, std::ios::binary);
    char buffer[8192]; // Adjust buffer size as needed
    do {
        inputFile.read(buffer, sizeof(buffer));
        int bytesRead = inputFile.gcount();
        if (bytesRead > 0)
            send(clientSocket, buffer, bytesRead, 0);
    } while (!inputFile.eof());
    inputFile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: sender <image_folder_path>" << std::endl;
        return 1;
    }

    std::string folderpath = argv[1];
    const char* serverIP = "10.28.0.62"; // Replace with the actual IP address

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }

    // Create socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return 1;
    }

    // Server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(54000);
    if (inet_pton(AF_INET, serverIP, &(serverAddress.sin_addr)) != 1) {
        std::cerr << "Invalid server IP address" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Connect to the server
    if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&serverAddress), sizeof(serverAddress)) != 0) {
        std::cerr << "Failed to connect to the server" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Read the image file
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(folderpath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::string imagePath = folderpath + "\\" + findData.cFileName;
                sendimage(imagePath, clientSocket);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    // Clean up
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}