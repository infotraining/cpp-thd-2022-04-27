#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <variant>

using namespace std::literals;

template <typename T>
using ThreadResult = std::variant<std::monostate, T, std::exception_ptr>;

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay, 
                     ThreadResult<char>& result)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    try
    {
        //throw std::runtime_error("ERROR#13");
        //result = text.at(3); // throws exception
    }
    catch(...)
    {
        std::cout << "Caught an exception in THD::id = " << std::this_thread::get_id() << std::endl;
        result = std::current_exception();
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

template <typename... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};

// deduction guide
template <typename... Ts>
overload(Ts...) -> overload<Ts...>;

//////////////////////////////////
// How it works

struct Lambda_86782356782365
{
    auto operator()(int x) const { return 42 * x; }
};

struct Lambda_86782356785345
{
    auto operator()(std::exception_ptr excpt_ptr) {
            try
            {
                std::rethrow_exception(excpt_ptr);
            }
            catch(const std::out_of_range& e)
            {
                std::cout << "Caught an exception: " << e.what() << std::endl;
            }
            catch(const std::exception& e)
            {
                std::cout << "Caught a generic exception " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cout << "Caught something..." << std::endl;
            }
        }
};

struct Overload : Lambda_86782356782365, Lambda_86782356785345
{
    using Lambda_86782356782365::operator();
    using Lambda_86782356785345::operator();
};

int main()
{
    auto l = [](int x) { return 42 * x; };

    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    ThreadResult<char> result;

    std::thread thd_1{&background_work, 1, "Hello", 250ms, 
                      std::ref(result)};
    thd_1.join();

    std::visit(overload{
        [](std::monostate) { std::cout << "No value!!!" << std::endl; },
        [](char value) { std::cout << "Value: " << value << std::endl; },
        [](std::exception_ptr excpt_ptr) {
            try
            {
                std::rethrow_exception(excpt_ptr);
            }
            catch(const std::out_of_range& e)
            {
                std::cout << "Caught an exception: " << e.what() << std::endl;
            }
            catch(const std::exception& e)
            {
                std::cout << "Caught a generic exception " << e.what() << std::endl;
            }
            catch(...)
            {
                std::cout << "Caught something..." << std::endl;
            }
        }
    }, result);
    

    // if (excpt_ptr)
    // {
    //     try
    //     {
    //         std::rethrow_exception(excpt_ptr);
    //     }
    //     catch(const std::out_of_range& e)
    //     {
    //         std::cout << "Caught an exception: " << e.what() << std::endl;
    //     }
    //     catch(const std::exception& e)
    //     {
    //         std::cout << "Caught a generic exception " << e.what() << std::endl;
    //     }
    //     catch(...)
    //     {
    //         std::cout << "Caught something..." << std::endl;
    //     }
    // }
    // else
    // {
    //     std::cout << "Value: " << value << std::endl;        
    // }

    std::cout << "Main thread ends..." << std::endl;
}
