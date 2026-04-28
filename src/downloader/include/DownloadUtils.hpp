/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    DownloadUtils.hpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   Header to include download definitions
 */

#ifndef DOWNLOAD_UTILS_HPP
#define DOWNLOAD_UTILS_HPP

#include "DownloadDefinitions.hpp"
namespace mbient::dlmanager {

enum class HttpResponseCode : uint16_t {
    Unknown = 0,
    RequestSuccess = 200,
    RangeRequestSuccess = 206,
    BadRequest = 400,
    Forbidden = 403,
    ProxyAuthenticationRequired = 407,
    UriTooLong = 414,
    RangeNotSatisfiable = 416,
    TooManyRequests = 429,
    InternalServerError = 500,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HttpVersionNotSupported = 505,
    NetworkAuthenticationRequired = 511
};

/**
 * @brief Given a DownloadReturnCode enum value and an error message, returns a
 * DownloadResult object.
 *
 * @param code DownloadReturnCode enum value describing the outcome of the
 * download operation
 * @param error Error message
 * @return DownloadResult
 */
DownloadResult generateDownloadResult(const DownloadReturnCode& code, const std::string& error);

/**
 * @brief Given an HttpResponseCode enum value, returns an error message
 * describing the protocol error.
 *
 * @param responseCode HttpResponseCode enum value
 * @return std::string Error message desciribing the Http protocol error that
 * corresponds to the given HttpResponseCode enum value
 */
std::string getHttpProtocolError(HttpResponseCode responseCode);

} // namespace mbient::dlmanager

#endif // DOWNLOAD_UTILS_HPP