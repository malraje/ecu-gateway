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

// Server.hpp
#pragma once

#ifndef SERVER_HPP
#define SERVER_HPP

#include "CGWDownloader.hpp"

#include <boost/signals2/connection.hpp>
#include <glib.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class CloudGateway;
class IStopToken;

// Enum for message types
enum class MessageType : int8_t { Data = 1, Status = 2, invalid = -1 };
class GatewayServer {
public:
    GatewayServer();
    GatewayServer(int port, std::shared_ptr<IStopToken> stopToken);
    ~GatewayServer();

    void start();
    void handleClient(int& clientSocket);
    gboolean acceptClient();
    bool isValidUrlFormat(const std::string& url);
    void processUrl(const std::string& url);
    void stop();
    bool sendDownloadResultToRSU(
        uint32_t jobId, const std::string& status, uint16_t errorCodeValue);
    bool transferChunkToRSU(const char* source, uint64_t maxSize);
    void setCloudGateway(std::shared_ptr<CloudGateway> gateway);
    bool sendMessageToRSU(
        MessageType messageType,
        const void* payload,
        size_t payloadSize,
        uint16_t errorCodeValue = 0,
        std::function<bool(const char*, long int)> confirmationHandler = nullptr);

    void processShutdown();

    void closeConnection(int socketId);
    // Callback function to be triggered after the timeout
    static gboolean timeoutCallbackWrapper(gpointer userData)
    {
        auto* callback = reinterpret_cast<std::function<gboolean()>*>(userData);
        gboolean result = (*callback)();
        return result;
    }

private:
    int m_port;
    mbient::ecugateway::Logger m_log;
    typedef std::function<gboolean()> CallbackFunction;
    int m_clientSocket = -1;
    bool m_downloadInProgress = false;
    std::shared_ptr<CloudGateway> m_cloudgateway;
    std::shared_ptr<IStopToken> m_stopToken;
    int m_serverSocket;
    std::atomic<bool> m_stopped;
    boost::signals2::connection m_stopConnection;
};
std::shared_ptr<GatewayServer> createGatewayServerInsatnce(
    int port, std::shared_ptr<IStopToken> stopToken);
#endif // SERVER_HPP