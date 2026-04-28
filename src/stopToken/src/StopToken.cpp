// SPDX-FileCopyrightText: copyright (c) 2022 MBition GmbH
// All rights reserved.
// SPDX-License-Identifier: LicenseRef-mbition-mbition-proprietary

/**
 * @file    StopToken.cpp
 * @brief   Implementation of StopToken and StopSource
 *          Poor implementation of c++20 stop_source and stop_token. It is intended
 *          to separate CGW part which reacts to interruption requests and
 *          parts of the service which should cooperate in order to abort its current activity
 */

#include "StopToken.hpp"

#include "Logger.hpp"

#include <boost/format.hpp>
#include <boost/signals2/signal.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class StopSourceImpl;

std::string getDescription(const std::string& description)
{
    return (boost::format("[%1%] ") % description).str();
}

std::string getDescription(const std::string& parent, const std::string& description)
{
    return (boost::format("[%1%][%2%] ") % parent % description).str();
}

class StopTokenImpl : public IStopToken {
    friend StopSourceImpl;

public:
    StopTokenImpl(const std::string& parent, const std::string& description)
        : m_description(getDescription(parent, description))
    {
    }

    bool is_interrupted() override
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        return m_interrupted;
    }

    boost::signals2::connection connect(boost::function<void()>&& slot) override
    {
        LOG(INFO, m_description << "connecting slot");
        boost::signals2::connection connection;
        bool already_triggered{false};
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            connection = m_sig.connect(slot);
            if (m_interrupted) {
                already_triggered = true;
            }
        }

        if (already_triggered) {
            // trigger slot if interruption happened before client even subscribed to it
            // call slot directly
            slot();
        }
        LOG(INFO, "No of connected slots" << m_sig.num_slots());

        return connection;
    }

    std::string toString() const override
    {
        return m_description;
    }

private:
    void trigger()
    {
        bool already_triggered{false};

        {
            std::lock_guard<std::mutex> lock{m_mutex};
            if (m_interrupted) {
                already_triggered = true;
            } else {
                m_interrupted = true;
            }
        }

        if (!already_triggered) {
            LOG(INFO, m_description << "stop token is trigerred");
            m_sig();
        } else {
            LOG(WARN, m_description << "stop token is already being handled");
        }
    }

    void reset()
    {
        LOG(INFO, m_description << "reseting token");
        // TODO: resetting slots would be a good idea,
        // but some will be remaining connected even after shutdown
        m_interrupted = false;
        m_sig.disconnect_all_slots();
    }

    const std::string m_description;
    using Signal = boost::signals2::signal<void()>;
    std::mutex m_mutex;
    bool m_interrupted = false;
    Signal m_sig;
    mbient::ecugateway::Logger m_log;
};

class StopSourceImpl : public IStopSource {
public:
    StopSourceImpl(const std::string& description)
        : m_description(description)
        , m_token(std::make_shared<StopTokenImpl>(description, "token"))
    {
    }

    void trigger() override
    {
        bool expected = false;
        if (m_stopped.compare_exchange_weak(expected, true)) {
            LOG(INFO, "stopping stop source runtime");
            m_token->trigger();
        } else {
            LOG(INFO, "stop source already stopped");
        }
    }

    std::shared_ptr<IStopToken> token() override
    {
        return m_token;
    }

    void reset() override
    {
        m_stopped = false;
        m_token->reset();
    }

    ~StopSourceImpl() override = default;

private:
    const std::string m_description;
    std::atomic<bool> m_stopped = false;
    std::mutex m_mutex;
    std::shared_ptr<StopTokenImpl> m_token;
    mbient::ecugateway::Logger m_log;
};

std::shared_ptr<IStopSource> createStopSource(const std::string& description)
{
    return std::make_shared<StopSourceImpl>(description);
}
