#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <mutex>
#include <future>
using namespace std;

namespace ver_1_0
{
    void run_calculation(long N, long& result)
    {
        auto thd_id = std::this_thread::get_id();
        std::mt19937_64 rand_engine {std::hash<std::thread::id>()(thd_id)};
        std::uniform_real_distribution<double> rand_distr {0, 1.0};

        long hits {};

        for (long n = 0; n < N; ++n)
        {
            double x = rand_distr(rand_engine);
            double y = rand_distr(rand_engine);
            if (x * x + y * y < 1)
                hits++;
        }

        result = hits;
    }
}

namespace ver_2_0
{
    void run_calculation(long N, long& result)
    {
        auto thd_id = std::this_thread::get_id();
        std::mt19937_64 rand_engine {std::hash<std::thread::id>()(thd_id)};
        std::uniform_real_distribution<double> rand_distr {0, 1.0};

        for (long n = 0; n < N; ++n)
        {
            double x = rand_distr(rand_engine);
            double y = rand_distr(rand_engine);
            if (x * x + y * y < 1)
                result++;
        }
    }
}


template<typename T>
struct Synchronized
{
    T value;
    std::mutex mtx;
};


void countPi(long int countsPerThread, std::atomic<long>& hits)
{
    auto thd_id = std::this_thread::get_id();
    std::mt19937_64 rand_engine {std::hash<std::thread::id>()(thd_id)};
    std::uniform_real_distribution<double> rand_distr {0, 1.0};

    for (long n = 0; n < countsPerThread; ++n)
    {
        double x = rand_distr(rand_engine);
        double y = rand_distr(rand_engine);
        if (x * x + y * y < 1)
        {
            //++hits;
            hits.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void countPi(long int countsPerThread, Synchronized<long>& value)
{
    auto thd_id = std::this_thread::get_id();
    std::mt19937_64 rand_engine {std::hash<std::thread::id>()(thd_id)};
    std::uniform_real_distribution<double> rand_distr {0, 1.0};

    for (long n = 0; n < countsPerThread; ++n)
    {
        double x = rand_distr(rand_engine);
        double y = rand_distr(rand_engine);
        if (x * x + y * y < 1) {
            lock_guard lk{value.mtx};
            value.value++;
        }
    }
}

double calc_pi_single_thread(long N)
{
    long hits {};
    ver_2_0::run_calculation(N, hits);
    return static_cast<double>(hits) / N * 4;
}

double calc_pi_multithreading(long N)
{
    auto num_of_threads = std::thread::hardware_concurrency();

    std::vector<long> results(num_of_threads);
    std::vector<std::thread> threads {};

    for (int i = 0; i < num_of_threads; i++)
    {
        threads.emplace_back(&ver_2_0::run_calculation, N / num_of_threads, std::ref(results[i]));
    }

    for (auto& thd : threads)
    {
        if (thd.joinable())
            thd.join();
    }

    auto hits = std::accumulate(results.begin(), results.end(), 0);

    return static_cast<double>(hits) / N * 4;
}

struct AlignedForCacheValue
{
    alignas(64) long value;
};

double calc_pi_multithreading_with_padding(long N)
{
    auto num_of_threads = std::thread::hardware_concurrency();

    std::vector<AlignedForCacheValue> results(num_of_threads);
    std::vector<std::thread> threads {};

    for (int i = 0; i < num_of_threads; i++)
    {
        threads.emplace_back(&ver_2_0::run_calculation, N / num_of_threads, std::ref(results[i].value));
    }

    for (auto& thd : threads)
    {
        if (thd.joinable())
            thd.join();
    }

    auto hits = std::accumulate(results.begin(), results.end(), 0, [](long r, AlignedForCacheValue v)
        { return r + v.value; });

    return static_cast<double>(hits) / N * 4;
}

double calc_pi_atomic(long N)
{
    auto num_of_threads = std::thread::hardware_concurrency();
    const long countsPerThread = static_cast<const long>(N / num_of_threads);

    std::vector<std::thread> threads;

    std::atomic<long> hits(0);

    for (int x = 0; x < num_of_threads; x++)
        threads.emplace_back([&] { countPi(countsPerThread, std::ref(hits)); });

    for (auto& ele : threads)
        if (ele.joinable())
            ele.join();

    const double pi = static_cast<double>(hits) / N * 4;

    return pi;
}

double calc_pi_mutex(long N)
{
    auto num_of_threads = std::thread::hardware_concurrency();
    const long countsPerThread = static_cast<const long>(N / num_of_threads);

    std::vector<std::thread> threads;

    Synchronized<long> hits{};

    for (int x = 0; x < num_of_threads; x++)
        threads.emplace_back([&]{ countPi(countsPerThread, std::ref(hits)); });

    for (auto& ele : threads)
        if (ele.joinable())
            ele.join();

    const double pi = static_cast<double>(hits.value) / N * 4;

    return pi;
}

namespace fut
{
long run_calculation(long N) 
{

  auto thd_id = std::this_thread::get_id();
  std::mt19937_64 rand_engine{std::hash<std::thread::id>()(thd_id)};
  std::uniform_real_distribution<double> rand_distr{0, 1.0};

  long result = 0;

    for (long n = 0; n < N; ++n) {
    double x = rand_distr(rand_engine);
    double y = rand_distr(rand_engine);
    if (x * x + y * y < 1)
        result++;
  }
  return result;
}

double calc_pi_multithreading(long N)
{
    auto num_of_threads = std::thread::hardware_concurrency();
    
    std::vector<std::future<long>> futures;

    for (int i = 0; i < num_of_threads; i++)
    {
        futures.push_back(std::async(&run_calculation, N / num_of_threads));
    }

    long result {0};

    for (auto& fut : futures)
    {
        result += fut.get();
    }

    return static_cast<double>(result) / N * 4;
}
} // namespace fut

int main()
{
    const long N = 100'000'000;

    //////////////////////////////////////////////////////////////////////////////
    // single thread
    {
        cout << "Pi calculation started (ST)!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        double pi = calc_pi_single_thread(N);

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }

    //////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////
    // multithreading thread
    {
        cout << "Pi calculation started (MT false sharing)!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        double pi = calc_pi_multithreading(N);

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }

    //////////////////////////////////////////////////////////////////////////////
    // multithreading thread
    {
        cout << "Pi calculation started (MT padding)!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        double pi = calc_pi_multithreading_with_padding(N);

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }

    //////////////////////////////////////
    // mutex
    {
        cout << "Pi calculation started (MT mutex)!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        double pi = calc_pi_mutex(N);

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }

         //////////////////////////////////////////////////////////////////////////////
    // future multithreading thread
    {
        std::cout << "Pi calculation started (future)!" << endl;
        const auto start = chrono::high_resolution_clock::now();

        double pi = fut::calc_pi_multithreading(N);

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        std::cout << "Pi = " << pi << endl;
        std::cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }  
}
