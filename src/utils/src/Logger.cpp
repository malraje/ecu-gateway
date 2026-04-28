/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    LogInitializer.hpp
 * @author  Mallikarjun Hanganalli (mallikarjun.hanganalli@mercedes-benz.com)
 */

#include "Logger.hpp"

#include <dltappender/loggingmacros.hpp>

namespace mbient {
namespace ecugateway {

Logger::Logger()
    : log4cplus::Logger(log4cplus::Logger::getRoot())
{
}

Logger::Logger(const std::string& loggerName)
    : log4cplus::Logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(loggerName)))
{
}

std::vector<std::string_view> getSplittedLogs(std::string_view logMessage)
{
    constexpr ssize_t substringMessageLength = (DLT_USER_BUF_MAX_SIZE - 100);
    size_t k = 0;
    std::vector<std::string_view> splittedLogs;

    for (size_t toProcess = logMessage.length(); k < logMessage.length();
         toProcess = logMessage.length() - k) {
        if (toProcess < substringMessageLength) {
            splittedLogs.emplace_back(logMessage.substr(k));
            break;
        }
        // find a candidate and split at the last space in the block
        // if no space is found, truncate anyway
        auto blockCandidate = logMessage.substr(k, substringMessageLength);
        auto lastSpace = blockCandidate.rfind(' ');
        if (lastSpace == std::string_view::npos) {
            splittedLogs.emplace_back(blockCandidate);
            k += blockCandidate.length();
            continue;
        }

        splittedLogs.emplace_back(blockCandidate.substr(0, lastSpace));
        // + 1 to skip the space
        k += lastSpace + 1;
    }

    return splittedLogs;
}

void Logger::KPI_MARK([[maybe_unused]] const std::string& mark)
{
#ifndef PRODUCTION_RELEASE
    KPI_MARKER(KPI_PARTITION_MMB, KPI_DOMAIN_APP, mark);
#endif
}
} // namespace ecugateway
} // namespace mbient
