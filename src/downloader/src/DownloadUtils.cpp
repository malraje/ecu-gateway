/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    DownloadUtils.cpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   Implementation for download definitions
 */

#include "DownloadUtils.hpp"

#include <sstream>
#include <unordered_map>

// int g_mclientSocket;

namespace mbient::dlmanager {

const std::unordered_map<DownloadReturnCode, std::string> DownloadErcMap
    = {{DownloadReturnCode::GENERAL_FAILURE, "GENERAL_FAILURE"},
       {DownloadReturnCode::CURL_FAILURE, "CURL_FAILURE"},
       {DownloadReturnCode::PROTOCOL_FAILURE, "PROTOCOL_FAILURE"}};

const std::unordered_map<HttpResponseCode, std::string> HttpProtocolErcMap
    = {{HttpResponseCode::RequestSuccess, "RequestSuccess"},
       {HttpResponseCode::RangeRequestSuccess, "RangeRequestSuccess"},
       {HttpResponseCode::BadRequest, "BadRequest"},
       {HttpResponseCode::Forbidden, "Forbidden"},
       {HttpResponseCode::ProxyAuthenticationRequired, "ProxyAuthenticationRequired"},
       {HttpResponseCode::UriTooLong, "UriTooLong"},
       {HttpResponseCode::RangeNotSatisfiable, "RangeNotSatisfiable"},
       {HttpResponseCode::TooManyRequests, "TooManyRequests"},
       {HttpResponseCode::InternalServerError, "InternalServerError"},
       {HttpResponseCode::BadGateway, "BadGateway"},
       {HttpResponseCode::ServiceUnavailable, "ServiceUnavailable"},
       {HttpResponseCode::GatewayTimeout, "GatewayTimeout"},
       {HttpResponseCode::HttpVersionNotSupported, "HttpVersionNotSupported"},
       {HttpResponseCode::NetworkAuthenticationRequired, "NetworkAuthenticationRequired"}};

DownloadResult generateDownloadResult(const DownloadReturnCode& code, const std::string& error)
{
    std::stringstream errorStream;
    if (DownloadErcMap.find(code) != DownloadErcMap.end()) {
        errorStream << "error: " << DownloadErcMap.at(code) << ", ";
    }
    errorStream << "message: " << error;
    return DownloadResult{code, errorStream.str()};
}

std::string getHttpProtocolError(HttpResponseCode responseCode)
{
    std::stringstream errorStream;
    if (HttpProtocolErcMap.find(responseCode) == HttpProtocolErcMap.end()) {
        errorStream << "UnknownResponseCode (" << static_cast<uint16_t>(responseCode) << ")";
        return errorStream.str();
    }

    errorStream << HttpProtocolErcMap.at(responseCode) << " (HTTP "
                << static_cast<uint16_t>(responseCode) << ")";
    return errorStream.str();
}

} // namespace mbient::dlmanager