#ifndef STOP_TOKEN_HPP
#define STOP_TOKEN_HPP

#include <atomic>
#include <memory>

namespace ext
{
    struct SharedState
    {
        std::atomic<bool> stop_requested_ {false};
    };

    class StopToken
    {
        friend class StopSource;

        std::shared_ptr<SharedState> shared_state_;

        StopToken(std::shared_ptr<SharedState> shared_state)
            : shared_state_ {shared_state}
        {
        }

    public:
        StopToken() = default;

        bool stop_possible() const
        {
            if (shared_state_)
                return true;
            return false;
        }

        bool stop_requested() const
        {
            return shared_state_ ? shared_state_->stop_requested_.load() : false;
        }
    };

    class StopSource
    {
        std::shared_ptr<SharedState> shared_state_;

    public:
        StopSource()
            : shared_state_ {std::make_shared<SharedState>()}
        {
        }

        [[nodiscard]] StopToken get_token()
        {
            return StopToken {shared_state_};
        }

        void request_stop() const
        {
            shared_state_->stop_requested_ = true;
        }
    };

}

#endif