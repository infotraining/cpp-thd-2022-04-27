#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <numeric>
#include <algorithm>
#include <atomic>

using namespace std::literals;

class Data
{
    std::vector<int> data_;
    std::atomic<bool> is_data_ready_{false};
public:
    void produce()
    {
        std::cout << "Preparing data..." << std::endl;
        data_.resize(1000);
        std::random_device rd;
        std::mt19937_64 rnd_engine{rd()};
        std::uniform_int_distribution<int> rnd_distr(0, 100);
        std::generate(data_.begin(), data_.end(), [&] { return rnd_distr(rnd_engine);});
        std::this_thread::sleep_for(2s);
        std::cout << "Data ready..." << std::endl;
        
        int x = 32;  
        
        /////////////////////
        is_data_ready_.store(true, std::memory_order_relaxed); 

    }

    void consume(int id)
    {    
        int y = 665;

        while(!is_data_ready_.load(std::memory_order_relaxed)) ////// MB
        {}

        int sum = std::accumulate(data_.begin(), data_.end(), 0);
        std::cout << "Consumer#" << id << " - sum: " << sum << std::endl;
    }
};


int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    Data data;

    std::thread thd_producer{[&data] { data.produce(); }};
    std::thread thd_consumer_1{[&data] { data.consume(1); }};
    std::thread thd_consumer_2{[&data] { data.consume(2); }};

    thd_producer.join();
    thd_consumer_1.join();
    thd_consumer_2.join();

    std::cout << "Main thread ends..." << std::endl;    
}
