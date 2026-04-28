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

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <log4cplus/loggingmacros.h>
#include <unistd.h>

#include <iomanip>
#include <optional>
#include <string_view>

#define LOG_DID(value)                                                                             \
    "0x" << std::setfill('0') << std::setw(4) << std::hex << std::uppercase << (value)

/**
 * @brief helper function to extract file name from
 * the absolute source code path.
 */
constexpr const char* getFileName(const char* path)
{
    const char* file = path;
    while (*path != 0) {
        if (*path++ == '/') {
            file = path;
        }
    }
    return file;
}

namespace mbient {
namespace ecugateway {

/**
 * @brief logging function. Must be used in free function.
 * Is not supposed to be used in classes inheritet from Logger class.
 * @param TAG type of log. Can be WARN, TRACE, DEBUG, ERROR, INFO
 */
#define LOG_FUNCTION(TAG, ...)                                                                     \
    LOG4CPLUS_##TAG(                                                                               \
        log4cplus::Logger::getRoot(),                                                              \
        LOG4CPLUS_TEXT("[" << gettid() << "] " << getFileName(__FILE__))                           \
            << ":" << __LINE__ << " " << __VA_ARGS__)

/**
 * @brief logging function. Must be used in classes with Logger instance.
 * WARNING: Logger instance must have name "m_log".
 * @param TAG type of log. Can be WARN, TRACE, DEBUG, ERROR, INFO
 */
#define LOG(TAG, ...)                                                                              \
    LOG4CPLUS_##TAG(                                                                               \
        m_log,                                                                                     \
        LOG4CPLUS_TEXT("[" << gettid() << "] " << getFileName(__FILE__))                           \
            << ":" << __LINE__ << " " << __VA_ARGS__)

/**
 * @brief logging function to log function name
 * WARNING: Logger instance must have name "m_log".
 * @param TAG type of log. Can be WARN, TRACE, DEBUG, ERROR, INFO
 */
#define LOG_FUNCTION_ENTRY(TAG) LOG(TAG, __func__ << "()")

/**
 * @brief macro to log long messages in DLT and to avoid message truncation,
 * split preferably on spaces
 */
#define LOG_MULTI_LINE_MESSAGE(TAG, message)                                                       \
    do {                                                                                           \
        auto _msg = message;                                                                       \
        for (auto splittedLog : getSplittedLogs(_msg)) {                                           \
            LOG(TAG, splittedLog);                                                                 \
        }                                                                                          \
    } while (0)

#define DLT_USER_BUF_MAX_SIZE 1300

std::vector<std::string_view> getSplittedLogs(std::string_view logMessage);

/**
 * @brief log4plus class wrapper. All classes that use
 * log4plus must inherited from it.
 */
class Logger : public log4cplus::Logger {
public:
    /**
     * @brief constructor logger name
     * @param loggerName string logger name
     */
    Logger(const std::string& loggerName);

    /**
     * @brief constructor by default
     */
    Logger();

    // TODO: it is a workaround for APRICOTQAL-382
    //       because we cannot expose macros from dlt directly
    static void KPI_MARK(const std::string& mark);

    ~Logger() override = default;
};

} // namespace ecugateway
} // namespace mbient

#endif // LOGGER_HPP
