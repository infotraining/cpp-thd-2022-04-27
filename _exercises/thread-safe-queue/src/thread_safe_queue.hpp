#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue
{
    std::queue<T> q_;
    mutable std::mutex mtx_q_;
    std::condition_variable cv_q_not_empty_;
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    bool empty() const
    {
        std::lock_guard<std::mutex> lk{mtx_q_};
        return q_.empty();
    }

    void push(const T& item)
    {
        {
            std::lock_guard<std::mutex> lk{mtx_q_};
            q_.push(item);
        }

        cv_q_not_empty_.notify_one();
    }

    void push(std::initializer_list<T> items)
    {
        {
            std::lock_guard<std::mutex> lk{mtx_q_};
            for(const auto& item : items)
                q_.push(item);
        }

        cv_q_not_empty_.notify_all();
    }   

    // non-blocking operation - returns false when queue is empty 
    // or its mutex is locked by another thread
    bool try_pop(T& item)
    {        
        std::unique_lock<std::mutex> lk{mtx_q_, std::try_to_lock};
        
        if (lk.owns_lock() && !q_.empty())
        {
            item = q_.front();
            q_.pop();
            
            return true;
        }

        return false;
    }

    // blocking operation - waits if queue is empty
    void pop(T& item)
    {
        std::unique_lock<std::mutex> lk{mtx_q_};
        cv_q_not_empty_.wait(lk, [this] { return !q_.empty();});
        item = q_.front();
        q_.pop();        
    } 
};

#endif // THREAD_SAFE_QUEUE_HPP
