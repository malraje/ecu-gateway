/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    Server.hpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   implementation for Gateway server
 */

#include "Server.hpp"

#include "DownloadUtils.hpp"
#include "StopToken.hpp"

#include <fcntl.h>
#include <glib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#define URL_BUFFER_SIZE 1024
#define IS_HTTP_URL(url) ((url).compare(0, 7, "http://") == 0)
#define IS_HTTPS_URL(url) ((url).compare(0, 8, "https://") == 0)
#define DESTINATION_PATH "/mnt/swup/sample.txt"
#define FILE_PATH "buffer.txt"
#define CLIENT_TYPE "RSU"

#define EXTRACT_LOW_32(x) ((uint32_t)((x) & 0xFFFFFFFF))
#define EXTRACT_HIGH_32(x) ((uint32_t)(((x) >> 32) & 0xFFFFFFFF))
#define COMBINE_TO_64(low, high) (((uint64_t)(low) << 32) | (high))
#define HTONLL(x) COMBINE_TO_64(htonl(EXTRACT_LOW_32(x)), htonl(EXTRACT_HIGH_32(x)))

GatewayServer::GatewayServer() = default;

GatewayServer::GatewayServer(int port, std::shared_ptr<IStopToken> stopToken)
    : m_port(port)
    , m_stopToken(stopToken)
    , m_serverSocket(-1)
{
    // Create socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == -1) {
        LOG(ERROR, "Error creating socket");
        throw std::runtime_error("Error creating socket");
    }

    // Enable SO_REUSEADDR to allow fast reuse of the socket
    int opt = 1;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 5;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));

    // Bind to localhost and specified port
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(m_port);

    if (bind(m_serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        LOG(ERROR, "Error binding socket");
        shutdown(m_serverSocket, SHUT_RDWR);
        close(m_serverSocket);
        m_serverSocket = -1; // Prevents dangling descriptor
        throw std::runtime_error("Error binding socket");
    }
}

GatewayServer::~GatewayServer()
{
    close(m_serverSocket);
}

void GatewayServer::start()
{
    // connect to shutdown handler
    m_stopConnection = m_stopToken->connect(std::bind(&GatewayServer::processShutdown, this));

    // Listen for incoming connections
    if (listen(m_serverSocket, SOMAXCONN) == -1) {
        LOG(ERROR, "Error listening on socket");
        throw std::runtime_error("Error listening on socket");
    }
    LOG(INFO, "Gateway listening on port: " << m_port);
    // Set the server socket to non-blocking mode
    if (fcntl(m_serverSocket, F_SETFL, O_NONBLOCK) == -1) {
        LOG(ERROR, "Error setting socket to non-blocking mode");
        throw std::runtime_error("Error setting socket to non-blocking mode");
    }
    // Add a timeout event to the main loop to attempt accepting clients
    CallbackFunction callback = std::bind(&GatewayServer::acceptClient, this);
    g_timeout_add(
        100,
        [](gpointer data) -> gboolean {
            auto* server = reinterpret_cast<GatewayServer*>(data);
            return server->acceptClient();
        },
        this);
}

void GatewayServer::stop()
{
    // Stop accepting new clients
    LOG(INFO, "GatewayServer has been stopped.");
    m_stopConnection.disconnect();
    close(m_serverSocket);
    m_serverSocket = -1;
}

gboolean GatewayServer::acceptClient()
{
    int serverSocket = m_serverSocket;
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG(ERROR, "Error accepting connection.");
        }
        return G_SOURCE_CONTINUE;
    }
    LOG(INFO, "Accepted client connection: " << clientSocket);
    handleClient(clientSocket);

    return G_SOURCE_CONTINUE;
}

bool GatewayServer::isValidUrlFormat(const std::string& url)
{
    return IS_HTTP_URL(url) || IS_HTTPS_URL(url);
}

void GatewayServer::processShutdown()
{
    LOG(INFO, "Shutdown is triggered, handling for Gateway RSU server");
    m_stopped = m_stopToken->is_interrupted();
}

void GatewayServer::processUrl(const std::string& url)
{

    std::string destination = DESTINATION_PATH;
    std::string filePath = FILE_PATH;
    const std::string clientType = CLIENT_TYPE;
    std::string offsetKeyword = "&offset=";
    size_t offsetPosition = url.find(offsetKeyword);

    if (offsetPosition != std::string::npos) {
        // Extract the URL and offset as separate strings
        std::string extractedUrl = url.substr(0, offsetPosition);
        std::string offset = url.substr(offsetPosition + offsetKeyword.length());

        // Output the results
        LOG(INFO, "URL: " << extractedUrl << " Offset: " << offset);
        LOG(INFO, "Calling the downloadFile function" << m_downloadInProgress);

        if (m_downloadInProgress) {
            LOG(INFO, "Old Job in the progress so aborting");
            auto ongoingJob = m_cloudgateway->getJobID();
            if (ongoingJob.has_value()) {
                m_cloudgateway->discardOldJobResult(true);
                LOG(INFO, "Aborting ongoing job: " << ongoingJob->id());
                m_cloudgateway->abortDownload(ongoingJob.value());

            } else {
                LOG(INFO, "No ongoing job to abort.");
            }
        }

        std::pair<std::optional<Mbient::CloudGateway::JobId>, std::string> result
            = m_cloudgateway->downloadFile(
                extractedUrl, destination, clientType, std::stoul(offset));

        if (result.first.has_value() && result.second.empty()) {
            LOG(INFO, "download successfully started: " << result.first->id());
            m_downloadInProgress = true;
        } else if (result.second == "CREATE_TARGET_FILE_FAILURE") {
            LOG(ERROR, "couldn't create target file for CGW download: ");
        } else {
            closeConnection(m_clientSocket);
            LOG(ERROR, "Error due to CGW " << result.second);
        }

    } else {
        LOG(ERROR, "Invalid URL format received from client (no offset): " << url);
        closeConnection(m_clientSocket);
    }
}

void GatewayServer::handleClient(int& clientSocket)
{
    // Read URL from client
    char urlBuffer[URL_BUFFER_SIZE] = {0};
    m_clientSocket = clientSocket;

    LOG(INFO, "handleClient client connection: " << m_clientSocket);
    ssize_t bytesRead = recv(m_clientSocket, urlBuffer, sizeof(urlBuffer), 0);
    if (bytesRead <= 0) {
        LOG(ERROR, "Error reading URL from client");
        closeConnection(m_clientSocket);
        return;
    }
    std::string url(urlBuffer, bytesRead);
    // Validate URL format
    if (!isValidUrlFormat(url)) {
        LOG(ERROR, "Invalid URL format received from client: " << url);
        closeConnection(m_clientSocket);
        return;
    }
    LOG(INFO, "Received valid URL from client: " << url);
    // Process the URL further (extract and handle offset, etc.)
    processUrl(url);
}

bool GatewayServer::transferChunkToRSU(const char* source, uint64_t maxSize)
{
    auto confirmationHandler = [&](const char* data, ssize_t length) -> bool {
        if (strncmp(data, "next-chunk", 10) == 0) {
            return true;
        } else if (strncmp(data, "abort", 5) == 0) {
            LOG(INFO, "Abort request received from client.");
            if (auto jobId = m_cloudgateway->getJobID(); jobId.has_value()) {
                m_cloudgateway->m_abortHandle = true;
                m_cloudgateway->abortDownload(jobId.value());
            }
            // don't close connection on abort
            return true;
        } else {
            LOG(ERROR, "Invalid confirmation message received from client.");
            closeConnection(m_clientSocket); // Ensure to close socket
            return false;
        }
    };
    // errorCodeValue = 0 for chunk transfer
    return sendMessageToRSU(MessageType::Data, source, maxSize, 0, confirmationHandler);
}

bool GatewayServer::sendDownloadResultToRSU(
    uint32_t jobId, const std::string& status, uint16_t errorCodeValue)
{
    m_downloadInProgress = false; // Reset flag
    m_cloudgateway->m_abortHandle = false;
    auto confirmationHandler = [&](const char* data, ssize_t length) -> bool {
        if (strncmp(data, "no-download", 11) == 0) {
            LOG(INFO, "Received no-download request");
            return true;
        } else if (strncmp(data, "next-url", 8) == 0) {
            LOG(INFO, "Received next-url download request");
            handleClient(m_clientSocket);
            return true;
        } else {
            LOG(ERROR, "Invalid confirmation message received from client.");
            closeConnection(m_clientSocket); // Ensure to close socket
            return false;
        }
    };
    bool success = sendMessageToRSU(
        MessageType::Status, status.c_str(), status.length(), errorCodeValue, confirmationHandler);

    return success ? 1 : 0;
}

bool GatewayServer::sendMessageToRSU(
    MessageType messageType,
    const void* payload,
    size_t payloadSize,
    uint16_t errorCodeValue,
    std::function<bool(const char*, long int)> confirmationHandler)
{

    if (!payload || payloadSize == 0 || m_clientSocket < 0) {
        LOG(ERROR, " sendMessageToRSU  " << m_clientSocket);
        return false;
    }
    try {
        size_t offset = 0;
        uint64_t packetSize = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint64_t) + payloadSize;
        std::vector<char> buffer(packetSize);

        buffer[offset] = static_cast<uint8_t>(messageType);
        offset += sizeof(uint8_t);

        uint16_t netErrorCode = htons(errorCodeValue);
        std::memcpy(buffer.data() + offset, &netErrorCode, sizeof(netErrorCode));
        offset += sizeof(netErrorCode);

        uint64_t netPacketSize = HTONLL(packetSize);
        std::memcpy(buffer.data() + offset, &netPacketSize, sizeof(netPacketSize));
        offset += sizeof(netPacketSize);

        std::memcpy(buffer.data() + offset, payload, payloadSize);

        if (send(m_clientSocket, buffer.data(), buffer.size(), 0) <= 0) {
            LOG(ERROR, "closed socket called in send ");
            closeConnection(m_clientSocket);
            return false;
        }

        std::vector<char> confirmationBuffer(1024);
        ssize_t confirmationRead
            = recv(m_clientSocket, confirmationBuffer.data(), confirmationBuffer.size(), 0);

        if (confirmationRead <= 0) {
            LOG(ERROR, "closed socket called in confirmationRead ");
            closeConnection(m_clientSocket);
            return false;
        }

        if (confirmationHandler) {
            return confirmationHandler(confirmationBuffer.data(), confirmationRead);
        }

        // Default confirmation handler: succeed on any non-zero response
        return confirmationRead > 0;

    } catch (const std::exception& e) {
        LOG(ERROR, "Exception in sendMessageToRSU: " << e.what());
        closeConnection(m_clientSocket);
        if (auto jobId = m_cloudgateway->getJobID(); jobId.has_value()) {
            m_cloudgateway->abortDownload(jobId.value());
        }
        return false;
    }
}

void GatewayServer::closeConnection(int socketId)
{
    LOG(INFO, "closeConnection called. " << socketId);
    if (socketId != -1) {
        shutdown(socketId, SHUT_RDWR); // Graceful shutdown
        ::close(socketId);
        m_clientSocket = -1;
        LOG(INFO, "Socket closed successfully. " << socketId);
    } else {
        LOG(ERROR, "Invalid socket ID received for closing. " << socketId);
    }
}

void GatewayServer::setCloudGateway(std::shared_ptr<CloudGateway> cloudgateway)
{
    if (!cloudgateway) {
        LOG(ERROR, "[GatewayServer] Error: CloudGateway pointer is null");
        return;
    }

    m_cloudgateway = cloudgateway;
}

std::shared_ptr<GatewayServer> createGatewayServerInsatnce(
    int port, std::shared_ptr<IStopToken> stopToken)
{
    return std::make_shared<GatewayServer>(port, stopToken);
}
