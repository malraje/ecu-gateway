/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    DownloadDefinitions.hpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   Header to include download definitions
 */

#ifndef DOWNLOAD_DEFINITIONS_HPP
#define DOWNLOAD_DEFINITIONS_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <string>
namespace mbient::dlmanager {

using ContentLength = uint64_t;
struct DownloadFileInfo {
    ContentLength contentLength;
    std::string contentType;
};

struct DownloadProgress {
    uint64_t dlTotal{0};
    uint64_t dlNow{0};
    uint16_t percent{0};

    auto tie() const
    {
        return std::tie(dlTotal, dlNow, percent);
    }

    bool operator==(const DownloadProgress& rhs) const
    {
        return tie() == rhs.tie();
    }
};

constexpr uint16_t DownloadErcComponentId = 19;
constexpr char DownloadErcComponentName[] = {"DOWNLOAD_WORKER"};

enum class DownloadReturnCode {
    SUCCESS = 0,
    ABORTED = 1,
    GENERAL_FAILURE = 2,
    CURL_FAILURE = 3,
    PROTOCOL_FAILURE = 4,
    PAUSED = 5
};

const std::map<uint16_t, std::string> DownloadReturnCodeMap
    = {{static_cast<uint16_t>(DownloadReturnCode::SUCCESS), "DOWNLOAD_SUCCESS"},
       {static_cast<uint16_t>(DownloadReturnCode::ABORTED), "DOWNLOAD_ABORTED"},
       {static_cast<uint16_t>(DownloadReturnCode::GENERAL_FAILURE), "DOWNLOAD_GENERAL_FAILURE"},
       {static_cast<uint16_t>(DownloadReturnCode::CURL_FAILURE), "DOWNLOAD_CURL_FAILURE"},
       {static_cast<uint16_t>(DownloadReturnCode::PROTOCOL_FAILURE), "DOWNLOAD_PROTOCOL_FAILURE"},
       {static_cast<uint16_t>(DownloadReturnCode::PAUSED), "DOWNLOAD_PAUSED"}};

struct DownloadResult {
    DownloadReturnCode code{DownloadReturnCode::GENERAL_FAILURE};
    std::string message;

    // Default constructor deleted to avoid false positive results. DownloadResult
    // must be created explicitly with initializers.
    DownloadResult() = delete;
    DownloadResult(DownloadReturnCode downloadReturnCode, std::string downloadResultMessage)
        : code(downloadReturnCode)
        , message(downloadResultMessage)
    {
    }
    bool operator==(const DownloadResult& downloadResult) const
    {
        return ((code == downloadResult.code) && (message == downloadResult.message));
    }
};

using DownloadId = uint32_t;
using DownloadResultCallback = std::function<void(const DownloadId&, const DownloadResult&)>;
using CGWDownloadResultCallback
    = std::function<void(const uint32_t&, const std::string&, uint16_t errorCodeValue)>;
using DownloadProgressCallback
    = std::function<void(const DownloadId&, const DownloadProgress& progress)>;

} // namespace mbient::dlmanager

#endif // DOWNLOAD_DEFINITIONS_HPP
