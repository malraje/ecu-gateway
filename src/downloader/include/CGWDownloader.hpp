/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    CGWDownloader.hpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   Header to include downloader definitions
 */

#ifndef CGW_DOWNLOADER_H
#define CGW_DOWNLOADER_H

#include "CGWEventListener.hpp"
#include "DownloadUtils.hpp"

#include <Mbient/CloudGateway.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace fs = std::filesystem;

namespace DownloadGateway {
class DownloadGatewayImpl; // Forward declaration if needed
}

class GatewayServer;
class DownloadBuffer;
class CloudGateway {
public:
    CloudGateway(
        std::shared_ptr<DownloadGateway::DownloadGatewayImpl> httpServer,
        std::shared_ptr<GatewayServer> gatewayServer);

    ~CloudGateway();

    bool createFile(const std::string& filePath);

    std::pair<std::optional<Mbient::CloudGateway::JobId>, std::string> downloadFile(
        const std::string& url,
        const std::string& filePath,
        const std::string clientType,
        const size_t offset = 0);

    void processAcknowledge(const Mbient::CloudGateway::JobId& jobId, const std::exception_ptr ex);

    void processAcknowledgement(const std::exception_ptr ex);

    void abortDownload(Mbient::CloudGateway::JobId jobid);

    void registerCloudGateway();

    void unregisterCloudGateway();

    int jobCompletionCallback(
        const uint32_t& jobId, const std::string& status, uint16_t errorCodeValue);

    int handleRSUCompletion(uint32_t jobId, const std::string& status, uint16_t errorCodeValue);

    int handleSNDCompletion(uint32_t jobId, const std::string& status);

    std::optional<Mbient::CloudGateway::JobId> getJobID();

    void discardOldJobResult(bool state);

    void setRsuDownloadBuffer(std::shared_ptr<DownloadBuffer> buffer);

private:
    std::optional<Mbient::CloudGateway::AsyncCall> m_callHandler;
    std::shared_ptr<CGWEventListener> m_eventListener;
    Mbient::CloudGateway::ApplicationManager m_cgwClient;
    std::shared_ptr<GatewayServer> m_gatewayServer;
    std::shared_ptr<DownloadGateway::DownloadGatewayImpl> m_httpServer;
    std::string m_appName;
    std::string m_appTag;
    std::vector<uint8_t> m_appID;
    Mbient::CloudGateway::ApplicationInfo m_appInfo;
    std::optional<Mbient::CloudGateway::JobId> m_jobID;
    mbient::ecugateway::Logger m_log;
    bool m_discardOldJobResult = false;
    std::shared_ptr<DownloadBuffer> rsuDownloadBuffer_;

public:
    Mbient::CloudGateway::IOResource* mIoRsrc{nullptr};
    bool m_abortHandle = false;
};

class DownloadBuffer : public Mbient::CloudGateway::BinaryBuffer {
public:
    DownloadBuffer(
        std::shared_ptr<GatewayServer> gatewayServer, std::shared_ptr<CloudGateway> cloudgateway)
        : m_gatewayServer(gatewayServer)
        , m_cloudGateway(cloudgateway)
    {
    }

    auto writeData(const char* source, const uint64_t maxSize) -> uint64_t override;

    ~DownloadBuffer() override = default;

private:
    mbient::ecugateway::Logger m_log;
    std::shared_ptr<GatewayServer> m_gatewayServer;
    std::shared_ptr<CloudGateway> m_cloudGateway;
};

std::shared_ptr<CloudGateway> createCloudGatewayInstance(
    std::shared_ptr<DownloadGateway::DownloadGatewayImpl> httpServer,
    std::shared_ptr<GatewayServer> gatewayServer);

#endif // CGW_DOWNLOADER_H
