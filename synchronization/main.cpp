#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "joining_thread.hpp"

using namespace std::literals;

void run(int& counter)
{
    for(int i = 0; i < 1'000'000; ++i)
        ++counter;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::cout << "Main thread ends..." << std::endl;
}
