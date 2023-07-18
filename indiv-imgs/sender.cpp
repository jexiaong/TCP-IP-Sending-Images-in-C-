#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <opencv2/opencv.hpp>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: sender <image_file_path>\n";
        return 1;
    }

    const char* imageFilePath = argv[1];
    const char* serverIP = "10.28.0.62"; // Replace with the actual IP address

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        return 1;
    }

    // Create socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        WSACleanup();
        return 1;
    }

    // Server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(54000);
    if (inet_pton(AF_INET, serverIP, &(serverAddress.sin_addr)) != 1) {
        std::cerr << "Invalid server IP address\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Connect to the server
    if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&serverAddress), sizeof(serverAddress)) != 0) {
        std::cerr << "Failed to connect to the server\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Read the image file
    std::ifstream file(imageFilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open image file: " << imageFilePath << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    //Load images and send to server
    std::ifstream inputFile(imageFilePath, std::ios::binary);
    char buffer[8192];
    do {
        inputFile.read(buffer, sizeof(buffer));
        int bytesRead = inputFile.gcount();
        if (bytesRead > 0) {
            send(clientSocket, buffer, bytesRead, 0);
        }
    } while (!inputFile.eof());
    inputFile.close();
    
    std::cout << "Image sent successfully\n";

    // Clean up
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
