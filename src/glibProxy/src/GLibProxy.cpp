/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    GLibProxy.cpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   implementation of glib library
 */

#include "HttpServer.hpp"
#include "IGLibProxy.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "StopToken.hpp"

#include <cassert>
#include <iostream>

// Concrete implementation of GLib proxy
class GLibProxy : public IGLibProxy {
public:
    GLibProxy(
        std::shared_ptr<IStopSource> stopSource,
        std::shared_ptr<DownloadGateway::DownloadGatewayImpl> httpServer,
        std::shared_ptr<GatewayServer> gatewayServer)
        : m_stopSource(stopSource)
        , m_httpServer(httpServer)
        , m_gatewayServer(gatewayServer)
    {
        Glib::init();
        Gio::init();
        m_mainLoop = Glib::MainLoop::create(Glib::MainContext::get_default());
    }

    void run() override
    {
        m_mainLoop->run();
    }

    void stop() override
    {
        m_mainLoop->quit();
    }

    void setTerminationHandler() override
    {
        const auto signalHandler = [](gpointer userData) {
            assert(userData);

            auto* glib = static_cast<GLibProxy*>(userData);
            if (glib->m_stopSource) {
                LOG_FUNCTION(INFO, "trigger shutdown");
                glib->m_stopSource->trigger();
            }

            if (glib->m_gatewayServer) {
                glib->m_gatewayServer->stop();
            }
            if (glib->m_httpServer) {
                glib->m_httpServer->stop();
            }
            // exit main thread and stop processing new glib events
            glib->stop();

            return G_SOURCE_REMOVE;
        };
        LOG(INFO, "Setting glib signal termination handlers");
        g_unix_signal_add(SIGINT, signalHandler, this);
        g_unix_signal_add(SIGTERM, signalHandler, this);
    }

    void setTimeout(std::function<bool()> func, std::chrono::milliseconds interval) override
    {
        Glib::signal_timeout().connect(func, static_cast<unsigned int>(interval.count()));
    }

private:
    Glib::RefPtr<Glib::MainLoop> m_mainLoop; // GLib main loop
    std::shared_ptr<IStopSource> m_stopSource;
    std::shared_ptr<DownloadGateway::DownloadGatewayImpl> m_httpServer;
    std::shared_ptr<GatewayServer> m_gatewayServer;
    mbient::ecugateway::Logger m_log;
};

// Factory function to create GLib proxy instance
std::unique_ptr<IGLibProxy> createGLibProxy(
    std::shared_ptr<IStopSource> stopSource,
    std::shared_ptr<DownloadGateway::DownloadGatewayImpl> httpServer,
    std::shared_ptr<GatewayServer> gatewayServer)
{
    return std::make_unique<GLibProxy>(stopSource, httpServer, gatewayServer);
}
