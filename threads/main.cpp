#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "joining_thread.hpp"

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

class BackgroundWork
{
    const int id_;
    const std::string text_;

public:
    BackgroundWork(int id, std::string text)
        : id_{id}
        , text_{std::move(text)}
    {
    }

    void operator()(std::chrono::milliseconds delay) const
    {
        std::cout << "BW#" << id_ << " has started..." << std::endl;

        for (const auto& c : text_)
        {
            std::cout << "BW#" << id_ << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        std::cout << "BW#" << id_ << " is finished..." << std::endl;
    }
};



int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::thread thd_empty;
    std::cout << thd_empty.get_id() << std::endl;

    ext::joining_thread thd_1{&background_work, 1, "Hello", 250ms};
    ext::joining_thread legal = std::move(thd_1); 

    std::thread thd_2{&background_work, 2, std::cref(text), 300ms};
    std::thread thd_3{BackgroundWork{3, "!!!"}, 500ms};
    BackgroundWork bw{4, "dupa"};
    std::thread thd_4{bw, 1s};
    std::thread thd_5{[text]{ background_work(5, text, 750ms); }}; // []<>(){}();

    //text.at(100);
    
    thd_2.join();
    thd_3.join();
    thd_4.detach();
    assert(thd_4.joinable() == false);
    thd_5.join();

    std::cout << "Main thread ends..." << std::endl;
}
