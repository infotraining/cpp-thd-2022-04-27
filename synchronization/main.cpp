#include "joining_thread.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

template <typename T>
struct Synchronized
{
    T value;
    std::mutex mtx_value;
};

template <typename F, typename T>
void apply(F f, Synchronized<T>& sync_value)
{
    std::lock_guard lk {sync_value.mtx_value};
    f(sync_value.value);
}

void run(Synchronized<uint64_t>& counter)
{
    for (uint64_t i = 0; i < 1'000'000ULL; ++i)
    {
        // std::lock_guard lk{mtx_counter}; // SC begins
        // ++counter;
        
        apply([&](uint64_t& i)
            { ++i; },
            counter);
    } // SC ends
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    Synchronized<uint64_t> counter{};

    {
        ext::joining_thread thd1 {run, std::ref(counter)};
        ext::joining_thread thd2 {[&counter]
            { run(counter); }};
    } // implicit join

    std::cout << "Counter: " << counter.value << std::endl;

    std::cout << "Main thread ends..." << std::endl;
}
