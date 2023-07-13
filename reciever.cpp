#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <opencv2/opencv.hpp>

#pragma comment(lib, "ws2_32.lib")

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }

    // Create socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return 1;
    }

    // Bind the socket
    struct sockaddr_in serverAddress, clientAddress;
    int clientAddrLen = sizeof(clientAddress);
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(54000);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 1) == SOCKET_ERROR) { // 1 connection, adjust as needed
        std::cerr << "Failed to listen" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening for incoming connections..." << std::endl;

    // Accept a client socket
    SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to accept client socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Successfully connected" << std::endl;

    //Receive the image data and save it to a file
    const int bufferSize = 8192; // Adjust buffer size as needed
    char buffer[bufferSize];
    std::ofstream outputFile("received_image.jpg", std::ios::binary);
    int bytesRead;
    do {
        bytesRead = recv(clientSocket, buffer, bufferSize, 0);
        if (bytesRead > 0)
            outputFile.write(buffer, bytesRead);
    } while (bytesRead > 0);
    outputFile.close();

    //Load and display the received image using OpenCV
    cv::Mat receivedImage = cv::imread("received_image.jpg");
    cv::imshow("Received Image", receivedImage);
    cv::waitKey(0);

    //Cleanup
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
