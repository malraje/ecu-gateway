/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    HttpServer.cpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   implementation of http server
 */
#include "HttpServer.hpp"

#include "Logger.hpp"
#include "StopToken.hpp"

#include <boost/algorithm/string.hpp>
#include <cpprest/http_client.h>
#include <cpprest/http_listener.h>
#include <curl/curl.h>

#include <iostream>
#include <regex>
#include <string>
#include <vector>

#define LISTENER_URL "http://10.172.1.91:23782"
// #define LISTENER_URL "http://127.0.0.1:8080"

#define DESTINATION_PATH "/mnt/swup/ecu-gateway-storage/"

enum class DownloadState { IN_PROGRESS, SUCCESS, FAILED };
std::atomic<DownloadState> downloadStatus(DownloadState::IN_PROGRESS);

namespace DownloadGateway {

DownloadGatewayImpl::DownloadGatewayImpl(std::shared_ptr<IStopToken> token)
    : m_listener(LISTENER_URL)
    , m_destinationPath(DESTINATION_PATH)
    , m_stopToken(token)
{
    LOG(INFO, "Http server constructor called: ");
}

void DownloadGatewayImpl::start()
{
    try {
        LOG(INFO, "Http server is started and listening  : ");
        m_stopConnection
            = m_stopToken->connect(std::bind(&DownloadGatewayImpl::processShutdown, this));

        m_sndStorageDirectory = createOrClearSnddirectory(m_destinationPath);

        m_listener.support(
            methods::POST,
            std::bind(&DownloadGatewayImpl::handlePost, this, std::placeholders::_1));
        m_listener.support(
            methods::GET, std::bind(&DownloadGatewayImpl::handleGet, this, std::placeholders::_1));

        m_listener.open()
            .then([]() { LOG_FUNCTION(WARN, "Http server started successfully :"); })
            .wait();
    } catch (const std::exception& e) {
        LOG_FUNCTION(ERROR, "Failed to start http server: " << e.what());
    }
}

void DownloadGatewayImpl::processShutdown()
{
    LOG(INFO, "Shutdown is triggered, handling for Gateway SND server");
    m_stopped = m_stopToken->is_interrupted();
}

void DownloadGatewayImpl::startDownload(const utility::string_t& downloadUrl)
{

    downloadStatus = DownloadState::IN_PROGRESS;
    const std::string clientType = "SND";

    // Validate URL
    if (downloadUrl.empty()) {
        LOG(ERROR, "Error: Download URL is empty.");
        downloadStatus = DownloadState::FAILED;
        return;
    }

    if (downloadUrl.find(U("http://")) != 0 && downloadUrl.find(U("https://")) != 0) {
        LOG(ERROR, "Invalid URL format. URL must start with 'http://' or 'https://'.");
        downloadStatus = DownloadState::FAILED;
        return;
    }
    LOG(INFO, "Starting download from URL:" << downloadUrl);

    auto filename = extractFilenameFromUrl(downloadUrl);
    if (!filename.has_value()) {
        LOG(ERROR, "Error: Failed to extract filename from URL.");
        downloadStatus = DownloadState::FAILED;
        return;
    }

    LOG(INFO, "File name Extracted from URL : " << filename.value());

    std::filesystem::path filePath = m_sndStorageDirectory / filename.value();

    std::string m_fileDestinationPath = filePath.string();

    LOG(INFO, "File name with path : " << m_fileDestinationPath);

    if (!m_cloudgateway) {
        LOG(ERROR, "Failed to create CloudGateway instance.");
        downloadStatus = DownloadState::FAILED;
        return;
    }

    std::pair<std::optional<Mbient::CloudGateway::JobId>, std::string> result
        = m_cloudgateway->downloadFile(downloadUrl, m_fileDestinationPath, clientType);

    if (result.first.has_value() && result.second.empty()) {
        LOG(INFO, "download successfully started: " << result.first->id());
    } else if (result.second == "CREATE_TARGET_FILE_FAILURE") {
        downloadStatus = DownloadState::FAILED;
        LOG(ERROR, "couldn't create target file for CGW download: ");
    } else {
        downloadStatus = DownloadState::FAILED;
        LOG(ERROR, "Error due to CGW " << result.second);
    }
}

void DownloadGatewayImpl::handlePost(http_request request)
{
    if (request.relative_uri().path() == U("/start_download")) {
        request.extract_vector()
            .then([=](std::vector<unsigned char> bodyData) {
                // Convert raw binary data to hex format
                std::ostringstream hexStream;
                for (unsigned char byte : bodyData) {
                    hexStream << std::hex << std::setw(2) << std::setfill('0')
                              << static_cast<int>(byte);
                }
                LOG(INFO, "Received POST body (hex):" << hexStream.str());
                std::string decodedBody(bodyData.begin(), bodyData.end());
                LOG(INFO, "Decoded Body: " << decodedBody);
                std::thread([this, decodedBody]() { startDownload(decodedBody); }).detach();
                request.reply(status_codes::Accepted, U("Download Started"));
            })
            .then([=](pplx::task<void> t) {
                try {
                    t.get();
                } catch (const std::exception& e) {
                    request.reply(status_codes::InternalError, U("Error processing request"));
                }
            });
    } else {
        request.reply(status_codes::NotFound, U("Invalid request"));
    }
}

void DownloadGatewayImpl::handleGet(http_request request)
{
    try {
        auto path = request.relative_uri().path();
        if (path == U("/download_status")) {
            switch (downloadStatus.load()) {
            case DownloadState::IN_PROGRESS:
                request.reply(status_codes::OK, U("Download In Progress"));
                break;
            case DownloadState::SUCCESS:
                request.reply(status_codes::OK, U("Download Complete"));
                break;
            case DownloadState::FAILED:
                request.reply(status_codes::InternalError, U("Download Failed"));
                break;
            default:
                request.reply(status_codes::BadRequest, U("Unknown Download State"));
                break;
            }
        } else if (path == U("/fetch_data")) {
            if (!request.headers().has(U("Range"))) {
                LOG(ERROR, "Error: Missing Range header.");
                request.reply(status_codes::BadRequest, U("Missing Range header"));
                return;
            }

            auto rangeHeader = request.headers()[U("Range")];
            int start = 0, end = -1;
            if (!parseRangeHeader(rangeHeader, start, end)) {
                LOG(ERROR, "Error: Invalid Range header format");
                request.reply(status_codes::BadRequest, U("Invalid Range header format"));
                return;
            }

            LOG(INFO, "Parsed Range: Start = " << start << ", End = " << end);

            request.extract_vector().then([=](std::vector<unsigned char> bodyData) {
                try {
                    std::ostringstream hexStream;
                    for (unsigned char byte : bodyData) {
                        hexStream << std::hex << std::setw(2) << std::setfill('0')
                                  << static_cast<int>(byte);
                    }
                    LOG(INFO, "Received GET body (hex): " << hexStream.str());

                    std::string decodedBody(bodyData.begin(), bodyData.end());
                    LOG(INFO, "Decoded Body: " << decodedBody);

                    auto filename = extractFilenameFromUrl(decodedBody);
                    if (!filename.has_value()) {
                        LOG(ERROR, "Error: Failed to extract filename from URL.");
                        request.reply(status_codes::InternalError, U("Failed to extract filename"));
                        return;
                    }

                    LOG(INFO, "Extracted Filename: " << filename.value());
                    std::filesystem::path filePath = m_sndStorageDirectory / filename.value();

                    if (filePath.empty() || filePath.filename().empty()) {
                        LOG(ERROR, "Error: Missing filename in request path");
                        request.reply(
                            status_codes::BadRequest, U("Missing filename in request path"));
                        return;
                    }

                    if (!std::filesystem::exists(filePath)) {
                        LOG(ERROR, "Error: File not found");
                        request.reply(status_codes::NotFound, U("File not found"));
                        return;
                    }

                    std::ifstream fileStream(filePath, std::ios::binary | std::ios::ate);
                    if (!fileStream) {
                        LOG(ERROR, "Error: Failed to open file");
                        request.reply(status_codes::InternalError, U("Failed to open file"));
                        return;
                    }

                    int fileSize = static_cast<int>(fileStream.tellg());

                    LOG(INFO, "Adjusted Range: Start = " << start << ", End = " << end);
                    LOG(INFO, "File Size: " << fileSize);

                    if (start >= fileSize || start > end || end == -1 || end >= fileSize) {
                        LOG(ERROR, "Error: Invalid range");
                        request.reply(status_codes::RangeNotSatisfiable, U("Invalid range"));
                        return;
                    }

                    std::vector<char> buffer = readDataFromFile(filePath, start, end);
                    if (buffer.empty()) {
                        LOG(ERROR, "Error: Failed to read file data");
                        request.reply(status_codes::InternalError, U("Failed to read file data"));
                        return;
                    }

                    http_response response(status_codes::PartialContent);
                    response.headers().add(
                        U("Content-Range"),
                        U("bytes ") + std::to_string(start) + U("-") + std::to_string(end) + U("/")
                            + std::to_string(fileSize));

                    response.set_body(
                        std::string(buffer.begin(), buffer.end()), U("application/octet-stream"));
                    request.reply(response);
                } catch (const std::exception& ex) {
                    LOG(ERROR, "Exception in fetch_data lambda: " << ex.what());
                    request.reply(status_codes::InternalError, U("Internal server error"));
                } catch (...) {
                    LOG(ERROR, "Unknown exception in fetch_data lambda");
                    request.reply(status_codes::InternalError, U("Unknown error occurred"));
                }
            });
        } else {
            LOG(ERROR, "Unknown GET endpoint: " << utility::conversions::to_utf8string(path));
            request.reply(status_codes::NotFound, U("Unknown endpoint"));
        }
    } catch (const std::exception& ex) {
        LOG(ERROR, "Unhandled exception in handleGet: " << ex.what());
        request.reply(status_codes::InternalError, U("Unhandled server error"));
    } catch (...) {
        LOG(ERROR, "Unknown exception in handleGet");
        request.reply(status_codes::InternalError, U("Unknown server error"));
    }
}

void DownloadGatewayImpl::stop()
{
    LOG(INFO, "Http server is stopped");
    m_listener.close().wait();
    m_stopConnection.disconnect();
}

void DownloadGatewayImpl::setCloudGateway(std::shared_ptr<CloudGateway> cloudgateway)
{
    if (!cloudgateway) {
        LOG(ERROR, "[DownloadGatewayImpl] Error: CloudGateway pointer is null");
        return;
    }

    m_cloudgateway = cloudgateway;
}

std::filesystem::path DownloadGatewayImpl::createOrClearSnddirectory(
    const std::filesystem::path& m_destinationPath)
{
    std::filesystem::path sndDirectory = m_destinationPath / "snd";

    if (std::filesystem::exists(sndDirectory) && std::filesystem::is_directory(sndDirectory)) {
        for (const auto& entry : std::filesystem::directory_iterator(sndDirectory)) {
            std::filesystem::remove_all(entry.path());
        }
        LOG(INFO, "Contents of sndDirectory cleared successfully! ");
    } else {
        if (!std::filesystem::create_directory(sndDirectory)) {
            LOG(ERROR, "Failed to create snddirectory: " << sndDirectory);
        }
    }

    return sndDirectory;
}

std::optional<std::string> DownloadGatewayImpl::extractFilenameFromUrl(const std::string& url)
{
    size_t lastSlashPos = url.find_last_of('/');
    size_t questionMarkPos = url.find('?');

    if (lastSlashPos != std::string::npos) {
        if (questionMarkPos != std::string::npos && lastSlashPos < questionMarkPos) {
            return url.substr(lastSlashPos + 1, questionMarkPos - lastSlashPos - 1);
        }
        return url.substr(lastSlashPos + 1); // Return filename if no query parameters
    }

    return std::nullopt; // Return failure (no filename found)
}

bool DownloadGatewayImpl::parseRangeHeader(const std::string& rangeHeader, int& start, int& end)
{
    std::regex rangePattern(R"(bytes=(\d*)-(\d*)?)"); // Match "bytes=start-end"
    std::smatch match;

    if (std::regex_match(rangeHeader, match, rangePattern)) {
        try {
            start = match[1].str().empty() ? 0 : std::stoi(match[1].str()); // Default start = 0
            end = match[2].str().empty() ? -1 : std::stoi(match[2].str()); // Default end = -1 (EOF)

            // Ensure valid range: end must be >= start (except when end == -1)
            if (end != -1 && end < start) {

                LOG(ERROR,
                    "Invalid range: end is less than start. Start=" << start << ", End=" << end);
                return false;
            }

            return true; //  Successfully parsed
        } catch (const std::exception& e) {
            LOG(ERROR, "Error parsing Range header: " << e.what());
        }
    } else {
        LOG(ERROR, "Invalid Range header format (missing dash): " << rangeHeader);
    }

    return false; //  Parsing failed
}

// Function to handle sending status for download request
void DownloadGatewayImpl::sendStatusForDownloadRequest(const std::string& status)
{
    LOG(INFO, "Sending download status to the SND: " << status);

    if (status == "Completed") {
        downloadStatus = DownloadState::SUCCESS;

    } else if (status == "Failed") {
        downloadStatus = DownloadState::FAILED;
    } else {
        downloadStatus = DownloadState::FAILED;
        LOG(ERROR, "Invalid download status: " << status);
    }
}

// Function to read data from file within the specified range
std::vector<char> DownloadGatewayImpl::readDataFromFile(
    const std::filesystem::path& filePath, size_t start, size_t end)
{
    std::vector<char> responseData;

    // Check if the range is valid
    if (start <= end) {
        // Open the file
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            // Seek to the starting position of the range
            file.seekg(start);

            // Calculate the length of the range
            size_t rangeLength = end - start + 1;

            // Read the data for the range into a vector
            responseData.resize(rangeLength);
            file.read(responseData.data(), rangeLength);

            // Close the file
            file.close();
        } else {
            // Failed to open file
            LOG(ERROR, "Failed to open file: " << filePath);
        }
    } else {
        // Invalid range
        LOG(ERROR, "Invalid range: " << start << " - " << end);
    }

    return responseData;
}

std::shared_ptr<DownloadGatewayImpl> createHttpServerInstance(std::shared_ptr<IStopToken> token)
{
    // Use a static shared_ptr to store the instance
    static std::shared_ptr<DownloadGatewayImpl> httpServerInstance;

    // Check if the instance has already been created
    if (!httpServerInstance) {
        // If not, create a new instance
        httpServerInstance = std::make_shared<DownloadGatewayImpl>(token);
    }

    return httpServerInstance;
}

} // namespace DownloadGateway