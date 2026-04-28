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

#ifndef LOG_INITIALIZER_HPP
#define LOG_INITIALIZER_HPP

#include "Logger.hpp"

#include <log4cplus/fileappender.h>
#include <log4cplus/initializer.h>
#include <log4cplus/loglevel.h>

#include <string>

namespace mbient {
namespace ecugateway {

/**
 * @brief Global log initalization class. Must be instantiated in main function.
 */
class LogInitializer {
public:
    LogInitializer(const LogInitializer&) = delete;
    LogInitializer& operator=(const LogInitializer&) = delete;

    /**
     * @brief constructor, default console log4cplus
     * @param logLevel log level
     */
    LogInitializer(const log4cplus::LogLevel logLevel = log4cplus::OFF_LOG_LEVEL);

    /**
     * @brief constructor,if configPath is valid, configure DLT log4cplus, otherwise fallback
     * @param configPath path to log4cplus config
     * @param fallbackLl fallback log level
     */
    LogInitializer(
        const std::string& configPath,
        const log4cplus::LogLevel fallbackLl = log4cplus::OFF_LOG_LEVEL);
};

} // namespace ecugateway
} // namespace mbient

#endif // LOG_INITIALIZER_HPP
