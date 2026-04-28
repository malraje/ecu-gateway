/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    CGWEventListener.hpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   Header to include download definitions
 */

#ifndef CGW_DOWNLOAD_EVENTLISTENER_H
#define CGW_DOWNLOAD_EVENTLISTENER_H

#include "DownloadDefinitions.hpp"
#include "DownloadUtils.hpp"
#include "Logger.hpp"

#include <Mbient/CloudGateway.h>
#include <sys/socket.h>

#include <iostream>
#include <thread>

class CGWEventListener : public Mbient::CloudGateway::Observer {
public:
    CGWEventListener() = default;
    ~CGWEventListener() override = default;

    static std::shared_ptr<CGWEventListener> create()
    {
        return std::make_shared<CGWEventListener>();
    }

    void payloadReceived(const Mbient::CloudGateway::ReceivedInfo& receivedInfo) override
    {
        LOG(INFO, "=> Server Response: Len = " << receivedInfo.payload().size());
        LOG(INFO,
            "Payload:" << std::string(
                receivedInfo.payload().begin(), receivedInfo.payload().end()));
    }

    void cloudConnectionStatusChanged(Mbient::CloudGateway::TransportState state) override
    {
        LOG(INFO, "Connection State changed :" << static_cast<int32_t>(state));
    }

    void registerOpStatus(const mbient::dlmanager::CGWDownloadResultCallback& opstatus)
    {
        m_opstatus = opstatus;
        LOG(INFO, "CGWEventListener registerOpStatus() called");
    }

    void operationStateChanged(
        const Mbient::CloudGateway::JobId& jobId,
        const Mbient::CloudGateway::OperationStatus& status) override
    {
        std::string state;
        uint16_t errorCodeValue = 0;
        /* TODO REWORK OF JOBID */
        LOG(INFO, "=> Server Response: JOB ID " << jobId.id());
        switch (status.operationState()) {
        case Mbient::CloudGateway::OperationState::Completed: {
            state = "Completed";

            LOG(INFO, "Job Status: Completed");
            if (m_opstatus) {
                m_opstatus(jobId.id(), state, errorCodeValue);
            }

        } break;
        case Mbient::CloudGateway::OperationState::Failed: {
            state = "Failed";
            errorCodeValue = static_cast<int32_t>(status.errorCode());
            LOG(INFO, "Job Status: Failed: " << status.errorMessage());
            m_opstatus(jobId.id(), state, errorCodeValue);
            switch (status.errorCode()) {
            case Mbient::CloudGateway::ErrorCode::TimeOut:
                LOG(INFO, "Exception ERR_TIME_OUT ");
                break;
            case Mbient::CloudGateway::ErrorCode::CouldNotResolveHostName:
                LOG(INFO, "Exception ERR_COULD_NOT_RESOLVE_HOST_NAME ");
                break;
            default:
                LOG(INFO, " ErrorCode value: = " << errorCodeValue);
            }

        } break;
        case Mbient::CloudGateway::OperationState::Queued: {
            state = "Queued";
            LOG(INFO, "Job Status: Queue");
        } break;
        case Mbient::CloudGateway::OperationState::Inprogress: {
            state = "Inprogress";
            LOG(INFO,
                "Job Status: Inprogress, Downloaded: "
                    << status.progressInfo().downloadedSize() << "/"
                    << status.progressInfo().totalDownloadSize());
        } break;
        case Mbient::CloudGateway::OperationState::Started: {
            state = "Started";
            LOG(INFO, "Job Status: Started");
        } break;
        default: {
            state = "UNKNOWN";
            LOG(INFO, "Job Status: Unknown");
        }
        }
        LOG(INFO,
            "Response From Server :\n "
                << std::string(status.serverResponse().begin(), status.serverResponse().end()));
    }

private:
    mbient::dlmanager::CGWDownloadResultCallback m_opstatus;
    mbient::ecugateway::Logger m_log;
};
#endif // CGW_DOWNLOAD_EVENTLISTENER_H
