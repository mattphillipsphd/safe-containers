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

using SafeChars = sa::SafeArray<char>;
using namespace std::chrono_literals;

template<class T>unsigned char identify(T&& v) {return v;}

constexpr int NUM_TEST_ITERS = 10;

void print_sc(const SafeChars& sc)
{
    std::string s;
    for (auto it=sc.cbegin(); it!=sc.cend(); ++it)
        s += *it;
    std::cout << s << std::endl;
}

template<typename IterType>
void print_sc(const IterType& cbegin, const IterType& cend)
{
    std::string s;
    for (auto it=cbegin; it!=cend; ++it)
    {
        s += *it;
    }
    std::cout << s << std::endl;
}

void broadcast(SafeChars& safe_chars, char c)
{
    for (int i=0; i<NUM_TEST_ITERS; ++i)
    {
        for (auto it=safe_chars.begin(); it!=safe_chars.end(); ++it)
        {
            *it = c;
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }

        print_sc(safe_chars);
    }
}

void broadcast_unsafe(SafeChars& safe_chars, char c)
{
    for (int i=0; i<NUM_TEST_ITERS; ++i)
    {
        for (auto it=safe_chars.unsafe_begin(); it!=safe_chars.unsafe_end();
                ++it)
        {
            *it = c;
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }

        print_sc(safe_chars);
    }
}

template<typename IterType>
void broadcast(IterType& begin, IterType& end, char c)
{
    for (int i=0; i<NUM_TEST_ITERS; ++i)
    {
        for (auto it=begin; it!=end; ++it)
        {
            *it = c;
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }

        print_sc<IterType>(begin, end);
    }
}


// g++ -std=c++20 -pthread safe_iter2.cpp -o ~/bin/safety/safe_iter2
int main(int argc, char** argv)
{
    const int N = (argc > 1) ? std::stoi( argv[1] ) : 20;


    std::string s{"HELLOWWORLDHOWAREYOU"};
    SafeChars safe_chars{ (SafeChars::size_type)s.length() };
    
    for (auto it=safe_chars.begin(); it!=safe_chars.end(); ++it)
    {
        size_t i = it - safe_chars.begin();
        *it = s[i];
    }

    print_sc(safe_chars);

    std::cout << "Starting SAFE iteration test..." << std::endl;

    std::thread t1{
        static_cast<void(*)(SafeChars&, char)>(&broadcast),
        std::ref(safe_chars), 
        '1'};
    std::thread t2{
        static_cast<void(*)(SafeChars&, char)>(&broadcast),
        std::ref(safe_chars), 
        '9'};
    
    t1.join();
    t2.join();

    std::cout << "...iterator test complete!  Output should be uniform rows " \
       "of 1s and 9s\n\n" << std::endl;

    std::cout << "Starting UNSAFE iteration test #1..." << std::endl;

    std::thread t3{
        static_cast<void(*)(SafeChars&, char)>(&broadcast_unsafe),
        std::ref(safe_chars), 
        '1'};
    std::thread t4{
        static_cast<void(*)(SafeChars&, char)>(&broadcast_unsafe),
        std::ref(safe_chars), 
        '9'};
    
    t3.join();
    t4.join();

    std::cout << "...iterator test complete!  Output should be jumbled 1s "\
        "and 9s\n\n" << std::endl;

    std::cout << "Starting UNSAFE iteration test #2..." << std::endl;

    auto unsafe_begin = safe_chars.unsafe_begin();
    auto unsafe_end = safe_chars.unsafe_end();
    std::thread t5{broadcast<SafeChars::Iterator>,  std::ref(unsafe_begin), 
        std::ref(unsafe_end), '1'};
    std::thread t6{broadcast<SafeChars::Iterator>,  std::ref(unsafe_begin), 
        std::ref(unsafe_end), '9'};
    
    t5.join();
    t6.join();

    std::cout << "...iterator test complete!  Output should be jumbled 1s "\
        "and 9s\n\n" << std::endl;

    std::cout << "Attempting to use safe iterators in an unsafe way.  This " \
       " should trigger an assert:\n" << std::endl;
    auto begin = safe_chars.begin();
    auto end = safe_chars.end();
    std::thread t7{broadcast<SafeChars::SafeIterator>,  std::ref(begin), 
        std::ref(end), '1'};
    std::thread t8{broadcast<SafeChars::SafeIterator>,  std::ref(begin), 
        std::ref(end), '9'};
    
    t7.join();
    t8.join();
    return 0;
}
