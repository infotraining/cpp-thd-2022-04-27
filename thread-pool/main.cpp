#include "thread_safe_queue.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <future>
#include <tuple>

using namespace std::literals;

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

class ThreadPool
{
public:
    using Task = std::function<void()>;

    ThreadPool(uint8_t num_of_threads = std::thread::hardware_concurrency())
    {
        for (uint8_t i {0}; i < num_of_threads; ++i)
        {
            thd_pool_.emplace_back([this]
                { run(); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool()
    {
        for (auto& t : thd_pool_)
        {
            queue_tasks_.push(end_of_work_);
        }
        for (auto& t : thd_pool_)
        {
            t.join();
        }
    }


    template <typename Callable>
    auto submit(Callable&& task)
    {
        // if (!task)
        //     throw std::invalid_argument("Empty function is not allowed");

        using ResultT = decltype(task());
        auto pt = std::make_shared<std::packaged_task<ResultT()>>(std::forward<Callable>(task));
        auto f = pt->get_future();
                
        queue_tasks_.push([pt] { (*pt)(); });
        return f;
    }

private:
    static inline const Task end_of_work_ {};

    void run()
    {
        Task task;
        while (true)
        {
            queue_tasks_.pop(task);
            if (task)
                task();
            else
                return;
        }
    }

    std::vector<std::thread> thd_pool_ {};
    ThreadSafeQueue<Task> queue_tasks_ {};
};

namespace ver_1_1
{
    class ThreadPool
    {
    public:
        using Task = std::function<void()>;

        ThreadPool(uint8_t num_of_threads = std::thread::hardware_concurrency())
        {
            for (uint8_t i {0}; i < num_of_threads; ++i)
            {
                thd_pool_.emplace_back([this]
                    { run(); });
            }
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            for (auto& t : thd_pool_)
            {
                queue_tasks_.push([this]
                    { end_of_work_ = true; });
            }

            for (auto& t : thd_pool_)
            {
                t.join();
            }
        }

        void submit(const Task& task)
        {
            queue_tasks_.push(task);
        }

    private:
        void run()
        {
            Task task;
            while (!end_of_work_)
            {
                queue_tasks_.pop(task);
                task();
            }
        }

        std::vector<std::thread> thd_pool_ {};
        ThreadSafeQueue<Task> queue_tasks_ {};
        std::atomic<bool> end_of_work_ {false};
    };
}

int calculate_square(int x)
{
    std::cout << "Starting calculation for " << x << " in " << std::this_thread::get_id() << std::endl;

    std::random_device rd;
    std::uniform_int_distribution<> distr(100, 5000);

    std::this_thread::sleep_for(std::chrono::milliseconds(distr(rd)));

    if (x % 3 == 0)
        throw std::runtime_error("Error#3");

    return x * x;
}

int main()
{
    ThreadPool thd_pool {6};

    for (int i = 1; i < 20; ++i)
        thd_pool.submit([=]
            { background_work(i, "Thread Pool#" + std::to_string(i), 100ms); });

    std::vector<std::tuple<int, std::future<int>>> f_squares;

    for (int i = 1; i < 20; ++i)
        f_squares.push_back(std::tuple(i, thd_pool.submit([i] { return calculate_square(i); })));


    for(auto& [i, fs]  : f_squares)
    {
        try
        {            
            int result = fs.get();
            std::cout << i << "*" << i << " = " << result << std::endl;
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }        
    }
}
