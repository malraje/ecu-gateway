#ifndef STOPTOKEN_H_
#define STOPTOKEN_H_

#include <boost/function.hpp>
#include <boost/signals2/connection.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>

class IStopToken;

/**
 * @brief StopSource is used to signal to StopTokens that interruption is in progress.
 */
class IStopSource {
public:
    /**
     * @brief signals to StopTokens that interruption is initiated
     */
    virtual void trigger() = 0;
    /**
     * @brief creates a token associated with this stop source
     * @return associated stop token
     */
    virtual std::shared_ptr<IStopToken> token() = 0;

    virtual void reset() = 0;
    /**
     * @brief Destroy the IStopSource object
     *
     */
    virtual ~IStopSource() = default;
};

/**
 * @brief A holder of StopToken can check if interruption was requested or subscribe to such
 * requests. At the moment all holders share the same token, but later it can be changed
 */
class IStopToken {
public:
    /**
     * @brief indicates if source requested interruption
     * @return true if interruption was requested
     */
    virtual bool is_interrupted() = 0;

    /**
     * @brief allows holder to subscribe using boost signal. Holders of tokens will be issued
     *        in the subscription order.
     * @param slot of holder to handle when interruption is triggered
     * @return slot signal connection to manage subscription lifespan on holder's side
     */
    [[nodiscard]] virtual boost::signals2::connection connect(boost::function<void()>&& slot) = 0;

    /**
     * @brief Destroy the IStopToken object
     *
     */
    virtual ~IStopToken() = default;

    virtual std::string toString() const = 0;
};

/**
 * @brief Exception to communicate to the caller that the operation was interrupted.
 *        It seems reasonable to interrupt a deeply nested call by throwing this exception.
 *        Probably both throwing and cathing it should be performed while
 *        passing a token from high level logic down to time consuming operations.
 */
class StopException : public std::runtime_error {
public:
    StopException(const char* msg)
        : std::runtime_error(msg)
    {
    }
};

std::shared_ptr<IStopSource> createStopSource(const std::string& description = "");

#endif // STOPTOKEN_H_