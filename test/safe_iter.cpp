#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <iterator>
// #include <latch>
#include <mutex>
#include <thread>
#include <vector>

#include "../log.h"
#include "../safe-containers/safe_array.h"

using namespace std::chrono_literals;

// Design goals: RAII iterators that are always valid wrt concurrency.  Only
// the following situations are possible:
// 1) No iterators exist
// 2) Exactly one write iterator exists
// 3) Multiple read iterators exist
//
// Or, behavior which in some respect achieves the same objective.
// But a factory method which blocks as long as the situation is invalid seems
// like a reasonable way to go.
// Starting off modifying https://gist.github.com/jeetsukumaran/307264

template<class T>unsigned char identify(T&& v) {return v;}

// g++-10 -std=c++20 -pthread safe_iter.cpp -o ~/bin/safety/safe_iter
int main(int argc, char** argv)
{
    const int N = (argc > 1) ? std::stoi( argv[1] ) : 20;

    using SA = SafeArray<int>;
    SA sa{N};

    Log& log = Log::instance(std::cout);

    auto writer = [&sa, &log]{ 
        int ct = 0;
        for (auto it=sa.unsafe_begin(); it!=sa.unsafe_end(); ++it)
            *it = ct++;
    };

    auto reader = [&sa, &log]{
        for (auto it=sa.unsafe_begin(); it!=sa.unsafe_end(); ++it)
            log.write( *it + ", " );
        log.write("\n");
    };

    std::cout << "Starting thread-UNSAFE iteration, may take a few seconds ..."
        << std::endl;
    std::jthread write{writer};
    std::jthread read{reader};

    write.detach();
    read.join();
    std::cout << "...Done" << std::endl;


    auto safe_writer = [&sa, &log]
    { 
        int ct = 0;
        const auto sa_begin = sa.begin();
        const auto sa_end = sa.end();
        const std::string s = std::to_string(sa.get_writer_ct())
            + "Writers, writing...\n";
        log.write(s);
        for (auto it=sa_begin; it!=sa_end; ++it)
            *it = 100*ct++;
        log.write("...Writing done\n");
    };

    auto safe_reader = [&sa, &log]
    {
        const auto sa_cbegin = sa.cbegin();
        const auto sa_cend = sa.cend();
        std::string const s = "Reader count: " 
            + std::to_string(sa.get_reader_ct()) + "\n";
        log.write(s);
        std::string s2;
        for (auto it=sa_cbegin; it!=sa_cend; ++it)
            s2 += *it + ", ";
        s2 += "\n";
        log.write(s2);
    };

    std::cout << "Starting thread-safe iteration, may take a few seconds ..."
        << std::endl;
    std::jthread safe_write{safe_writer};
    std::jthread safe_read{safe_reader};

    safe_write.detach();
    safe_read.join();
    std::cout << "...Done" << std::endl;

    std::cout << "Now, multiple reads and a write. Jumbled output is okay "
        "as long as all numbers displayed, and the writer is either first "
        "or last." << std::endl;

/* When there is compiler support for std::latch ...
    int const NUM_THREADS = 5;
    std::latch test_latch{NUM_THREADS};
    std::vector<std::jthread> threads;
    for (int i; i<NUM_THREADS; ++i)
    {
        if (i == NUM_THREADS/2)
            threads.push_back( std::jthread{ [&]{
                    test_latch.count_down();
                    safe_writer();}
                    } );
        else
            threads.push_back( std::jthread{ [&]{
                    test_latch.count_down();
                    safe_reader();}
                    } );
    }
    test_latch.wait();
*/

    std::jthread safe_read_1{safe_reader};
    std::jthread safe_read_2{safe_reader};
    std::jthread safe_write_1{safe_writer};
    std::jthread safe_read_3{safe_reader};
    std::jthread safe_read_4{safe_reader};

    safe_read_1.detach();
    safe_read_2.detach();
    safe_write_1.detach();
    safe_read_3.detach();
    safe_read_4.join();
    std::cout << "...Done" << std::endl;

    std::cout << "...and we're done." << std::endl;
    return 0;
}

