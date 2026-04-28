/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    CGWDownloader.cpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   implementation for download
 */
/*Knowing the register interface is deprecated from Cloud Gateway; therefore, upon official
 * announcement from CGW, the new API will be used, and the following pragmas will be removed. */

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "CGWDownloader.hpp"

#include "HttpServer.hpp"
#include "Server.hpp"

#include <boost/algorithm/string.hpp>
#include <netinet/in.h> // For ntohs and ntohl functions

#include <cstdint>

std::unordered_map<uint32_t, std::string> jobClientMap;

uint64_t DownloadBuffer::writeData(const char* source, const uint64_t maxSize)
{
    try {
        if (0 == maxSize || nullptr == source) {
            LOG(ERROR, "Invalid parameters: maxSize={} or source pointer is null" << maxSize);
            return 0;
        }
        if (m_cloudGateway->m_abortHandle == true) {
            LOG(INFO, "Ignoring the data as abort requested from RSU");
            return maxSize;
        }
        if (!m_gatewayServer) {
            LOG(ERROR, "m_gatewayServer is null, cannot transfer data to RSU");
            return 0;
        }
        if (m_gatewayServer->transferChunkToRSU(source, maxSize)) {
            return maxSize;
        } else {
            LOG(ERROR, "transferChunkToRSU failed to transfer data");
        }
    } catch (const std::exception& ex) {
        LOG(ERROR, "Exception in writeData: {}" << ex.what());
    } catch (...) {
        LOG(ERROR, "Unknown exception in writeData");
    }

    LOG(WARN, "Returning 0 bytes written due to failure");
    return 0;
}

CloudGateway::CloudGateway(
    std::shared_ptr<DownloadGateway::DownloadGatewayImpl> httpServer,
    std::shared_ptr<GatewayServer> gatewayServer)
    : m_eventListener(CGWEventListener::create())
    , m_gatewayServer(gatewayServer)
    , m_httpServer(httpServer)
{
    // CloudGateway constructor
}

CloudGateway::~CloudGateway()
{
    // Destructor implementation
    unregisterCloudGateway();
}

bool CloudGateway::createFile(const std::string& filePath)
{
    bool returnVal = false;
    try {
        fs::path path(filePath);
        std::ofstream fsFilePath{path};
        if (fs::exists(path)) {
            if (chmod(filePath.c_str(), 0777) == 0) {
                returnVal = true;
                LOG(INFO, "File created with permissions Successfully:");
            }
        }
    } catch (std::exception& e) {
        LOG(ERROR, "exception raised at createfile!!");
    }
    return returnVal;
}

std::pair<std::optional<Mbient::CloudGateway::JobId>, std::string> CloudGateway::downloadFile(
    const std::string& url,
    const std::string& filePath,
    const std::string clientType,
    const size_t offset)
{
    LOG(INFO, __func__ << "url : " << url << " filePath: " << filePath);

    Mbient::CloudGateway::CallOptions callOpts;
    Mbient::CloudGateway::HttpOptions httpOptions;
    httpOptions.setHttpMethod(Mbient::CloudGateway::HttpMethod::Get);
    callOpts.setHttpOptions(httpOptions);
    callOpts.setOffsetAddress(offset);

    std::stringstream errorStream;

    if (clientType == "SND") {
        // For SND, create the Mbient::CloudGateway::File
        LOG(INFO, " filePath before sending to CGW: " << filePath);
        mIoRsrc = new Mbient::CloudGateway::File(filePath);
    } else {
        LOG(INFO, "Streaming download for RSU: ");
        mIoRsrc = rsuDownloadBuffer_.get();
        LOG(INFO, __func__ << "offset value before sending to cgw:" << offset);
    }

    try {
        if (clientType == "RSU" || (clientType == "SND" && createFile(filePath))) {
            m_callHandler = m_cgwClient.download(
                url,
                mIoRsrc,
                [clientType,
                 this](const Mbient::CloudGateway::JobId& jobId, std::exception_ptr ex) {
                    m_jobID = std::move(jobId);
                    processAcknowledge(jobId, ex);
                    // Store the mapping between m_jobID's id and clientType
                    uint32_t jobIdValue = jobId.id();
                    jobClientMap[jobIdValue] = clientType;
                },
                callOpts);
            // Wait till m_cgwClient.download() call is completed
            LOG(INFO, "Wait for the download call to complete");
            m_callHandler->wait();
        } else {
            LOG(ERROR, "couldn't create target file for CGW download");
            errorStream << "CREATE_TARGET_FILE_FAILURE";
        }
    } catch (const Mbient::CloudGateway::Exception& ex) {
        errorStream << "Download Call Failed: " << ex.what();
        LOG(ERROR, errorStream.str());
    }

    return {m_jobID, errorStream.str()};
}

void CloudGateway::processAcknowledge(
    const Mbient::CloudGateway::JobId& jobId, const std::exception_ptr ex)
{
    if (ex) {
        try {
            std::rethrow_exception(ex);
        } catch (const Mbient::CloudGateway::Exception& ec) {
            LOG(ERROR, __func__ << "-> Call Ack : " << ec.what());
        }
    } else {
        LOG(INFO, __func__ << "-> Call Ack : SUCCESS. Job Id:" << jobId.id());
    }
}

void CloudGateway::processAcknowledgement(const std::exception_ptr ex)
{
    if (ex) {
        try {
            std::rethrow_exception(ex);
        } catch (const Mbient::CloudGateway::Exception& ec) {
            LOG(ERROR, __func__ << "-> Call Ack : " << ec.what());
        }
    } else {
        LOG(INFO, __func__ << "-> Call Ack : SUCCESS. Job Id");
    }
}

void CloudGateway::abortDownload(Mbient::CloudGateway::JobId jobid)
{
    m_cgwClient.cancelOperation(
        jobid, [this](std::exception_ptr ex) { processAcknowledgement(ex); });
}

void CloudGateway::discardOldJobResult(bool state)
{
    m_discardOldJobResult = state;
}

void CloudGateway::setRsuDownloadBuffer(std::shared_ptr<DownloadBuffer> buffer)
{
    rsuDownloadBuffer_ = buffer;
}

void CloudGateway::registerCloudGateway()
{
    m_appInfo.setApplicationName(m_appName);
    m_appInfo.setApplicationLogTag(m_appTag);
    m_eventListener->registerOpStatus(std::bind(
        &CloudGateway::jobCompletionCallback,
        this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3));

    try {
        m_callHandler = m_cgwClient.registerApplication(
            m_appInfo, m_eventListener.get(), [this](std::exception_ptr ex) {
                processAcknowledgement(ex);
            });
    } catch (const Mbient::CloudGateway::Exception& ex) {
        m_callHandler.reset();
        LOG(ERROR, __func__ << "Call Failed  : " << ex.what());
    }
}

void CloudGateway::unregisterCloudGateway()
{
    try {
        m_callHandler = m_cgwClient.unregisterApplication(
            [this](std::exception_ptr ex) { processAcknowledgement(ex); });
    } catch (const Mbient::CloudGateway::Exception& ex) {
        LOG(ERROR, __func__ << "Call Failed  : " << ex.what());
    }
}
std::optional<Mbient::CloudGateway::JobId> CloudGateway::getJobID()
{
    return m_jobID;
}

// Function to retrieve client type based on JobId
std::string getClientTypeForJob(const uint32_t& jobId)
{
    auto it = jobClientMap.find(jobId);
    if (it != jobClientMap.end()) {
        return it->second;
    }
    return ""; // Return empty string if not found
}

int CloudGateway::jobCompletionCallback(
    const uint32_t& jobId, const std::string& status, uint16_t errorCodeValue = 0)
{
    try {
        // Get the client type based on the job ID
        auto clientType = getClientTypeForJob(jobId);

        if (clientType == "RSU") {
            // Create a thread to handle RSU client
            std::thread rsuThread([this, jobId, status, errorCodeValue]() {
                try {
                    handleRSUCompletion(jobId, status, errorCodeValue);
                } catch (const std::exception& e) {
                    LOG(ERROR, "Exception in RSU thread: " << e.what());
                }
            });
            rsuThread.detach(); // Detach the thread to run independently
        } else if (clientType == "SND") {
            // Create a thread to handle SND client
            std::thread sndThread([this, jobId, status]() {
                try {
                    handleSNDCompletion(jobId, status);
                } catch (const std::exception& e) {
                    LOG(ERROR, "Exception in SND thread: " << e.what());
                }
            });
            sndThread.detach(); // Detach the thread to run independently
        }

        return 0;
    } catch (const std::exception& e) {
        LOG(ERROR, "Exception caught: " << e.what());
        return 0; // Or another appropriate value to indicate failure
    }
}

int CloudGateway::handleRSUCompletion(
    uint32_t jobId, const std::string& status, uint16_t errorCodeValue)
{
    if (m_discardOldJobResult) {
        LOG(INFO, __func__ << " Download was paused. Skipping result send.");
        m_discardOldJobResult = false;
        return 0;
    }
    auto result = m_gatewayServer->sendDownloadResultToRSU(jobId, status, errorCodeValue);
    if (result == 0) {
        LOG(ERROR, __func__ << " Failed to send download result to RSU for Job ID: " << jobId);
    } else {
        LOG(INFO, __func__ << " Successfully sent download result to RSU for Job ID: " << jobId);
    }

    return result;
}
// Function to handle completion status for SND client
int CloudGateway::handleSNDCompletion(uint32_t jobId, const std::string& status)
{
    // Logging job ID and status
    LOG(INFO, __func__ << " Job ID: " << jobId);
    LOG(INFO, "SND Download Result Message: " << status);

    //  auto httpServer = DownloadGateway::createHttpServerInstance();
    m_httpServer->sendStatusForDownloadRequest(status);
    return 0;
}

std::shared_ptr<CloudGateway> createCloudGatewayInstance(
    std::shared_ptr<DownloadGateway::DownloadGatewayImpl> httpServer,
    std::shared_ptr<GatewayServer> gatewayServer)
{
    // Use a static shared_ptr to store the instance
    static std::shared_ptr<CloudGateway> cloudGatewayInstance;

    // Check if the instance has already been created
    if (!cloudGatewayInstance) {
        // If not, create a new instance
        cloudGatewayInstance = std::make_shared<CloudGateway>(httpServer, gatewayServer);
    }

    return cloudGatewayInstance;
}
