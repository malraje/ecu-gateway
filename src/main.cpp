/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    main.cpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   implementation of http server and gateway server
 */

#include "ArchitectureDefinitions.hpp"
#include "CGWDownloader.hpp"
#include "HttpServer.hpp"
#include "IGLibProxy.hpp"
#include "LogInitializer.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "StopToken.hpp"

#include <fcntl.h>
#include <glib.h>
#include <log4cplus/loglevel.h>
#include <systemd/sd-daemon.h>

#include <csignal>
#include <ctime>
#include <filesystem>

using namespace mbient::ecugateway;
using namespace DownloadGateway;

constexpr int32_t RsuPort = 23780;
// constexpr int32_t RsuPort = 8080;

#define ZERO_MS std::chrono::milliseconds(0)
#define VALUE std::chrono::milliseconds(20000)

static std::unique_ptr<IGLibProxy> g_glibProxy;

struct ServerContext {
    std::shared_ptr<GatewayServer> gateway;
    std::shared_ptr<DownloadGatewayImpl> httpServer;
};

static int run(IGLibProxy& glib)
{
    // notify systemd that app is ready
    sd_notify(0, "READY=1");

    LOG_FUNCTION(INFO, "ECU Gateway Service running...");

    // Handle the shutdown procedure
    glib.setTerminationHandler();

    auto wdtTout = VALUE;
    if (wdtTout != ZERO_MS) {
        auto wdtFunc = []() {
            sd_notify(0, "WATCHDOG=1");
            LOG_FUNCTION(INFO, "== Watchdog kick! ==");
            return true;
        };
        glib.setTimeout(wdtFunc, std::chrono::duration_cast<std::chrono::milliseconds>(wdtTout));
    }

    glib.run();

    LOG_FUNCTION(INFO, "ECU Gateway service exiting...");

    return 0;
}

int main(int /*argc*/, const char* /*argv*/[])
{

    try {

        const std::string DefaultUtilsLogFilePath
            = "/etc/log4cplus/config/ecu-gateway-log4cplus-config.ini";

        const log4cplus::Initializer initializer;
        LogInitializer(DefaultUtilsLogFilePath, log4cplus::INFO_LOG_LEVEL);

        LOG_FUNCTION(INFO, "====== ECU Gateway Service startup ======");
        LOG_FUNCTION(INFO, "Service built from commit: " + EcugdGitHashValue);

        // create stop token
        auto stopSource = createStopSource("main");
        auto stopToken = stopSource->token(); // to be shared to all module

        auto httpServer = createHttpServerInstance(stopToken);
        auto gateway = createGatewayServerInsatnce(RsuPort, stopToken);

        // Create and run GLib main loop
        // Create glibProxy only once
        if (!g_glibProxy) {
            g_glibProxy = createGLibProxy(stopSource, httpServer, gateway);
        }

        // Intializing CloudGateway
        auto cloudGateway = createCloudGatewayInstance(httpServer, gateway);
        cloudGateway->registerCloudGateway();

        auto rsuDownloadBuffer = std::make_shared<DownloadBuffer>(gateway, cloudGateway);

        httpServer->setCloudGateway(cloudGateway);
        gateway->setCloudGateway(cloudGateway);
        cloudGateway->setRsuDownloadBuffer(rsuDownloadBuffer);

        // Start the ECU gateway service with GLib's main event loop
        gateway->start();
        httpServer->start();

        return run(*g_glibProxy);

    } catch (const std::exception& e) {
        LOG_FUNCTION(ERROR, "ECU Gateway Service Exception in main:: " << e.what());
        return 1; // Return a non-zero exit code to indicate an error.
    }

    return 0;
}
