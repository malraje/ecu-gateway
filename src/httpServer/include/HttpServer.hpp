/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    HttpServer.hpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   implementation of http server
 */

#ifndef CGW_HTTPSERVER_H
#define CGW_HTTPSERVER_H

#include "CGWDownloader.hpp"

#include <boost/signals2/connection.hpp>
#include <cpprest/http_listener.h>

#include <filesystem>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

class CloudGateway;
class IStopToken;
namespace DownloadGateway {

class DownloadGatewayImpl {
public:
    DownloadGatewayImpl(std::shared_ptr<IStopToken> token);
    DownloadGatewayImpl(
        const std::string& ipAddress,
        int port,
        const std::filesystem::path& dir,
        std::shared_ptr<IStopToken> token);
    void start();
    void stop();
    void sendStatusForDownloadRequest(const std::string& status);
    void setCloudGateway(std::shared_ptr<CloudGateway> gateway);
    void processShutdown();

private:
    int m_port;
    mbient::ecugateway::Logger m_log;
    web::http::experimental::listener::http_listener m_listener;
    std::shared_ptr<CloudGateway> m_cloudgateway;
    std::filesystem::path m_destinationPath; // Member variable for the destination path
    std::filesystem::path m_sndStorageDirectory;
    std::string m_fileDestinationPath; // Member variable for the full destination path
    std::shared_ptr<IStopToken> m_stopToken;
    std::atomic<bool> m_stopped;
    boost::signals2::connection m_stopConnection;
    std::optional<std::string> extractFilenameFromUrl(const std::string& url);
    std::vector<char> readDataFromFile(
        const std::filesystem::path& filePath, size_t start, size_t end);
    std::filesystem::path createOrClearSnddirectory(const std::filesystem::path& m_destinationPath);
    void handlePost(http_request request);
    void handleGet(http_request request);
    void startDownload(const utility::string_t& downloadUrl);
    bool parseRangeHeader(const std::string& rangeHeader, int& start, int& end);
};

std::shared_ptr<DownloadGatewayImpl> createHttpServerInstance(std::shared_ptr<IStopToken> token);

} // namespace DownloadGateway

#endif
