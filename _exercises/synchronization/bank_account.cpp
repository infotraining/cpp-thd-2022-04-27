#include <iostream>
#include <thread>
#include <mutex>

class BankAccount
{
    const int id_;
    double balance_;
    mutable std::recursive_mutex mtx_;

public:
    BankAccount(int id, double balance)
        : id_(id)
        , balance_(balance)
    {
    }

    void print() const
    {        
        std::cout << "Bank Account #" << id_ << "; Balance = " << balance() << std::endl;
    }

    void withdraw(double amount)
    {
        std::lock_guard lk{mtx_};
        balance_ -= amount;
    }

    void deposit(double amount)
    {
        std::lock_guard lk{mtx_};
        balance_ += amount;
    }

    int id() const
    {
        return id_;
    }

    double balance() const
    {
        std::lock_guard lk{mtx_};
        return balance_;
    }

    void transfer(BankAccount& to, double amount)
    {   
        // // ver_1
        // std::unique_lock lk_from{mtx_, std::defer_lock};
        // std::unique_lock lk_to{to.mtx_, std::defer_lock};    
        // std::lock(lk_from, lk_to); // deadlock free implementation
        // balance_ -= amount;
        // to.balance_ += amount;

        // // ver_2
        // std::lock(mtx_, to.mtx_); // deadlock free implementation
        // std::unique_lock lk_from{mtx_, std::adopt_lock};
        // std::unique_lock lk_to{to.mtx_, std::adopt_lock};    
        // balance_ -= amount;
        // to.balance_ += amount;

        // ver_3 (since C++17)
        std::scoped_lock lk{mtx_, to.mtx_};
        balance_ -= amount;
        to.balance_ += amount;
    }

    void lock()
    {
        mtx_.lock();
    }

    void unlock()
    {
        mtx_.unlock();
    }

    bool try_lock()
    {
        return mtx_.try_lock();
    }

    std::unique_lock<std::recursive_mutex> with_lock()
    {
        return std::unique_lock{mtx_};
    }
};

void make_withdraws(BankAccount& ba, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        ba.withdraw(1.0);
}

void make_deposits(BankAccount& ba, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        ba.deposit(1.0);
}

void make_transfer(BankAccount& from, BankAccount& to, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        from.transfer(to, 1.0);
}

int main()
{
    const int NO_OF_ITERS = 10'000'000;

    BankAccount ba1(1, 10'000);
    BankAccount ba2(2, 10'000);

    std::cout << "Before threads are started: ";
    ba1.print();
    ba2.print();

    std::thread thd1(&make_withdraws, std::ref(ba1), NO_OF_ITERS);
    std::thread thd2(&make_deposits, std::ref(ba1), NO_OF_ITERS);
    std::thread thd3(&make_transfer, std::ref(ba1), std::ref(ba2), NO_OF_ITERS);
    std::thread thd4(&make_transfer, std::ref(ba2), std::ref(ba1), NO_OF_ITERS);

    {
        //std::lock_guard lk{ba1}; // implcit ba1.mtx_.lock()
        auto lk = ba1.with_lock(); // SC begins
        ba1.deposit(100.0); // implcit ba1.mtx_.lock()
        ba1.withdraw(500.0);
        ba1.transfer(ba2, 1000.0);
    } // SC ends

    std::unique_lock lk{ba2, std::try_to_lock};
    if (lk.owns_lock())
    {
        std::cout << "Inside CS" << std::endl;
    }

    thd1.join();
    thd2.join();
    thd3.join();
    thd4.join();

    std::cout << "After all threads are done: ";
    ba1.print();
    ba2.print();
}
