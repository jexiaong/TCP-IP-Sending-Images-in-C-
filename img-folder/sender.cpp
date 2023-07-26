#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <vector>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

bool numericSort(const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
    std::string fileA = a.path().filename().string();
    std::string fileB = b.path().filename().string();

    //Remove non-digit characters from filename
    fileA.erase(std::remove_if(fileA.begin(), fileA.end(), [](char c) {
        return !std::isdigit(c);
    }), fileA.end());
    fileB.erase(std::remove_if(fileB.begin(), fileB.end(), [](char c) {
        return !std::isdigit(c);
    }), fileB.end());

    return std::stoi(fileA) < std::stoi(fileB);
}

uint32_t countsortFiles(const std::string &folderPath, std::vector<std::filesystem::directory_entry> &files) {
    if (!std::filesystem::is_directory(folderPath)) {
        std::cout << "Invalid folder path." << std::endl;
        return 1;
    }

    int fileCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            ++fileCount;
        }

        files.push_back(entry);
    }
    
    std::sort(files.begin(), files.end(), numericSort);

    return fileCount;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: sender <image_folder_path> <frequency_in_milliseconds>\n";
        return 1;
    }

    std::string folderPath = argv[1];
    uint32_t freq = std::stoi(argv[2]);
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

    // Send frequency
    if (send(clientSocket, (char*)&freq, sizeof(freq), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send frequency\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }    

    // Send total number of items and sort the folder
    std::vector<std::filesystem::directory_entry> files;
    uint32_t num = countsortFiles(folderPath, files);
    std::cout << num << std::endl;
    if (send(clientSocket, (char*)&num, sizeof(num), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send number of files\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    
    // Iterate over images in the folder
    for (const auto& entry : files) {
        std::string imagePath = entry.path().string();

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
        std::cout << "Image bytes: " << bytes << std::endl;
        if (send(clientSocket, (char*)&bytes, sizeof(bytes), 0) == SOCKET_ERROR) {
            std::cerr << "Failed to send bytes\n";
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        // Load and send the image to the server
        char buffer[65535]; //8192
        file.seekg(0, std::ios::beg); // Move the file pointer to the beginning
        while (!file.eof()) {
            file.read(buffer, sizeof(buffer));
            int bytesRead = file.gcount();
            if (bytesRead > 0) {
                send(clientSocket, buffer, bytesRead, 0);
            }
        }
        file.close(); // Close the file after sending
        std::cout << "Image sent successfully: " << imagePath << std::endl;
    }

    // Clean up
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
