#include "catch.hpp"
#include "stop_token.hpp"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace ext;

TEST_CASE("StopToken - stop possible")
{
    SECTION("when default constructed - stop is not possible")
    {
        StopToken st;

        REQUIRE(st.stop_possible() == false);
    }

    SECTION("when obtained from StopSource - stop is possible")
    {
        StopSource source;

        StopToken st = source.get_token();

        REQUIRE(st.stop_possible());
    }
}

TEST_CASE("StopToken - stop requested")
{
    SECTION("when default constructed - returns false")
    {
        StopToken st;
        REQUIRE(st.stop_requested() == false);
    }

    SECTION("when constructed from StopSource")
    {
        StopSource source;
        StopToken st = source.get_token();

        SECTION("returns false")
        {
            REQUIRE(st.stop_requested() == false);
        }

        SECTION("when stop requested on source - returns true")
        {
            source.request_stop();

            REQUIRE(st.stop_requested());
        }
    }
}

void run(StopToken stop_token)
{
    while (true)
    {
        if (stop_token.stop_requested())
            break;
    }
}

TEST_CASE("threads - test1")
{
    StopSource source;

    std::thread thd1(&run, source.get_token());
    std::thread thd2(&run, source.get_token());

    std::this_thread::sleep_for(500ms);

    source.request_stop();

    thd1.join();
    thd2.join();
}

TEST_CASE("threads - test2")
{
    StopSource source;

    std::thread thd1(&run, source.get_token());
    std::thread thd2(&run, source.get_token());
    std::thread thd3 {[source = std::move(source)]() mutable
        {
            std::this_thread::sleep_for(1s);
            source.request_stop();
        }};

    thd1.join();
    thd2.join();
    thd3.detach();
}
