#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <opencv2/opencv.hpp>
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: sender <image_folder_path>\n";
        return 1;
    }

    std::string folderpath = argv[1];
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
    
    // Iterate over images in the folder
    for (const auto& entry : std::filesystem::directory_iterator(folderpath)) {

        const std::string& imagePath = entry.path().string();

        // Read the image file
        std::ifstream file(imagePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open image file: " << imagePath << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        //Get the number of bytes of the image
        uint32_t bytes = static_cast<uint32_t>(file.tellg());
        std::cout << bytes << std::endl;
        if (send(clientSocket, (char*)&bytes, sizeof(bytes), 0) == SOCKET_ERROR) {
            std::cerr << "Failed to send bytes\n";
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        // Load and send the image to the server
        std::ifstream inputFile(imagePath, std::ios::binary);
        char buffer[8192];
        do {
            inputFile.read(buffer, sizeof(buffer));
            int bytesRead = inputFile.gcount();
            if (bytesRead > 0) {
                send(clientSocket, buffer, bytesRead, 0);
            }
        } while (!inputFile.eof());
        inputFile.close();

        std::cout << "Image sent successfully: " << imagePath << std::endl;
        
        Sleep(10000);
    }

    // Clean up
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
