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

#include "LogInitializer.hpp"

#include <boost/algorithm/string.hpp>
#include <dltappender/DltAppender.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/property.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <filesystem>
#include <iostream>

namespace mbient {
namespace ecugateway {

LogInitializer::LogInitializer(const log4cplus::LogLevel logLevel)
{
    log4cplus::Initializer initializer;
    log4cplus::helpers::Properties props;
    props.setProperty("log4cplus.rootLogger", "TRACE, EcuGatewayConsoleAppender");
    props.setProperty("log4cplus.appender.EcuGatewayConsoleAppender", "log4cplus::ConsoleAppender");
    props.setProperty(
        "log4cplus.appender.EcuGatewayConsoleAppender.layout", "log4cplus::PatternLayout");
    props.setProperty(
        "log4cplus.appender.EcuGatewayConsoleAppender.layout.ConversionPattern", "[%-5p][%d]%m%n");

    log4cplus::PropertyConfigurator conf(props);
    conf.configure();
    log4cplus::Logger::getRoot().setLogLevel(logLevel);
}

LogInitializer::LogInitializer(const std::string& configPath, const log4cplus::LogLevel fallbackLl)
{
    if (!configPath.empty() && std::filesystem::exists(configPath)) {
        log4cplus::DltAppender::registerAppender();
        log4cplus::PropertyConfigurator::doConfigure(configPath);
    } else {
        log4cplus::BasicConfigurator::doConfigure();
        auto log = log4cplus::Logger::getRoot();
        log.setLogLevel(fallbackLl);
    }
}
} // namespace ecugateway
} // namespace mbient
