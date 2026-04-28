/**********************************************************************
 *  Project        MBition ( MBiENT )
 *  (c) copyright  2024
 *  Company        MBition Mercedes Benz Innovation Lab
 *  All rights reserved
 **********************************************************************/

/**
 * @file    IGLibProxy.hpp
 * @author  Mallikarjun Patil (mallikarjun.rajendra@mercedes-benz.com)
 * @brief   implementation of glib library
 */

#ifndef I_GLIB_PROXY_H
#define I_GLIB_PROXY_H

#include <giomm.h>
#include <glib-unix.h>
#include <glibmm.h>

#include <memory>

class IStopSource;
namespace DownloadGateway {
class DownloadGatewayImpl; // Forward declaration if needed
}

class GatewayServer;
class IGLibProxy {
public:
    virtual ~IGLibProxy() = default;

    virtual void run() = 0;
    virtual void stop() = 0;
    virtual void setTerminationHandler() = 0;
    virtual void setTimeout(std::function<bool()> func, std::chrono::milliseconds interval) = 0;
};

std::unique_ptr<IGLibProxy> createGLibProxy(
    std::shared_ptr<IStopSource> stopSource,
    std::shared_ptr<DownloadGateway::DownloadGatewayImpl> httpServer,
    std::shared_ptr<GatewayServer> gatewayServer);
#endif
