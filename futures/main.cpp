#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

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

void save_to_file(const std::string& filename)
{
    std::cout << "Saving to file: " << filename << std::endl;

    std::this_thread::sleep_for(3s);

    std::cout << "File saved: " << filename << std::endl;
}

void wtf()
{
    auto f1 = std::async(std::launch::async, &save_to_file, "data1.txt");
    auto f2 = std::async(std::launch::async, &save_to_file, "data2.txt");
    auto f3 = std::async(std::launch::async, &save_to_file, "data3.txt");
    auto f4 = std::async(std::launch::async, &save_to_file, "data4.txt");
}

void future_async()
{
    std::future<int> f13 = std::async(std::launch::async, &calculate_square, 13);
    std::future<int> f30 = std::async(std::launch::deferred, []
        { return calculate_square(30); });
    std::future<void> fsave = std::async(std::launch::async, &save_to_file, "data.txt");

    while (fsave.wait_for(250ms) != std::future_status::ready)
        std::cout << "Waiting..." << std::endl;

    std::shared_future<int> sf13 = f13.share();

    std::thread thd {[&sf13]
        {
            std::cout << "1. From THD: " << sf13.get() << std::endl;
            std::cout << "2. From THD: " << sf13.get() << std::endl;
        }};

    int result_sq_13 = sf13.get();
    std::cout << "13 * 13 = " << result_sq_13 << "\n";

    try
    {
        int result_sq_30 = f30.get();
        std::cout << "30 * 30 = " << result_sq_30 << "\n";
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "Caught: " << e.what() << std::endl;
    }

    thd.join();
}

template <typename Callable>
auto launch_async(Callable&& callable)
{
    using ResultT = decltype(callable());

    std::packaged_task<ResultT()> pt {std::forward<Callable>(callable)};
    std::future<ResultT> f = pt.get_future();

    std::thread thd {std::move(pt)};
    thd.detach();

    return f;
}

std::future<int> no_wtf()
{
    launch_async([]
        { save_to_file("data1.txt"); });
    launch_async([]
        { save_to_file("data2.txt"); });
    launch_async([]
        { save_to_file("data3.txt"); });
    auto fs = launch_async([]
        { return calculate_square(19); });
    return fs;
}

void future_packaged_task()
{
    std::packaged_task<int()> pt {[]
        { return calculate_square(13); }};

    std::future<int> f13 = pt.get_future();

    std::thread thd {std::move(pt)};

    std::cout << "13 * 13 = " << f13.get() << std::endl;

    thd.join();

    std::cout << "-------\n";

    auto fs = no_wtf();
    std::cout << "19 * 19 = " << fs.get() << std::endl;

    std::this_thread::sleep_for(10s);
}

class Calculator
{
public:
    std::future<int> get_future()
    {
        return promise_.get_future();
    }

    void calculate(int n)
    {
        try
        {
            int result = calculate_square(n);
            promise_.set_value(result);
        }
        catch(...)
        {
            promise_.set_exception(std::current_exception());
        }        
    }

private:
    std::promise<int> promise_;
};

int main()
{
    Calculator calc;
    auto fs = calc.get_future();
    fs = calc.get_future();
    std::thread thd{[&calc] { calc.calculate(19); }};
    
    std::cout << "19 * 19 = " << fs.get() << std::endl;

    thd.join();
}
