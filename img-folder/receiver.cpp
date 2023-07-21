#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <filesystem>
#include <queue>
#include <mutex>
#include <condition_variable>

#pragma comment(lib, "ws2_32.lib")

std::mutex imgBytesMutex;
std::mutex displayMutex;
std::queue<cv::Mat> imageQueue;
std::condition_variable queueNotEmpty;
bool allImagesProcessed = false; // Flag to indicate when all images have been processed

void receiveBytes(const SOCKET& clientSocket, uint32_t& imgBytes) {
    std::lock_guard<std::mutex> lock(imgBytesMutex);
    recv(clientSocket, (char*)&imgBytes, sizeof(imgBytes), 0);
    std::cout << "Image bytes: " << imgBytes << std::endl;
}

void receiveAndQueueImage(const SOCKET& clientSocket, uint32_t numImages) {
    try {
        for (uint32_t i = 0; i < numImages; ++i) {
            // Receive the number of image bytes
            uint32_t imgBytes;
            receiveBytes(clientSocket, imgBytes);

            // Buffer to store the received image bytes
            std::vector<char> buffer(imgBytes);

            // Receive the image data
            int totalReceived = 0;
            int received;
            while (totalReceived < imgBytes) {
                received = recv(clientSocket, buffer.data() + totalReceived, imgBytes - totalReceived, 0);
                if (received == 0) {
                    break;
                }
                totalReceived += received;
            }

            // Decode the received image and enqueue it
            cv::Mat receivedImage = cv::imdecode(cv::Mat(buffer), cv::IMREAD_COLOR);
            if (!receivedImage.empty()) {
                std::lock_guard<std::mutex> lock(displayMutex);
                imageQueue.push(receivedImage);
                queueNotEmpty.notify_one();
            }

            if (i == numImages - 1) {
                allImagesProcessed = true; // Indicate that all images have been processed
                queueNotEmpty.notify_one(); // Notify the display thread to check the flag
            }
        }
    }
    catch (const cv::Exception& e) {
        std::cerr << "OpenCV exception occurred: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }
}

void displayImages(uint32_t freq) {
    while (true) {
        auto start = std::chrono::steady_clock::now();

        std::unique_lock<std::mutex> lock(displayMutex);
        queueNotEmpty.wait(lock, [] { return !imageQueue.empty() || allImagesProcessed; });

        // If the queue is empty and all images are processed, terminate the display thread
        if (imageQueue.empty() && allImagesProcessed) {
            break;
        }

        cv::Mat imageToDisplay = imageQueue.front();
        imageQueue.pop();
        lock.unlock();

        cv::imshow("Received Image", imageToDisplay);
        cv::waitKey(1);

        // Calculate elapsed and remaining time
        auto end = std::chrono::steady_clock::now();
        auto remaining = std::chrono::milliseconds(freq) -
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << remaining.count() << std::endl;
        if (remaining.count() > 0) {
            std::this_thread::sleep_for(remaining);
        }
    }
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        return 1;
    }

    // Create socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        WSACleanup();
        return 1;
    }

    // Bind the socket
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(54000);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind socket\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 1) == SOCKET_ERROR) { // 1 connection, adjust as needed
        std::cerr << "Failed to listen\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Server listening for incoming connections...\n";

    // Accept a client socket
    struct sockaddr_in clientAddress;
    int clientAddrLen = sizeof(clientAddress);
    SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to accept client socket\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Successfully connected\n";

    //Frequency of displaying new images as specified  by the sender commandline input
    uint32_t freq;
    recv(clientSocket, (char*)&freq, sizeof(freq), 0);

    // Total number of images in the folder
    uint32_t numImages;
    recv(clientSocket, (char*)&numImages, sizeof(numImages), 0);

    // Create a vector to store threads
    std::vector<std::thread> threads;

    // Start the image receiving and queuing thread
    threads.emplace_back(receiveAndQueueImage, clientSocket, numImages);

    // Start the image display thread
    threads.emplace_back(displayImages, freq);

    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }

    // Cleanup
    cv::destroyAllWindows();
    WSACleanup();
    closesocket(clientSocket);
    closesocket(serverSocket);
    return 0;
}
