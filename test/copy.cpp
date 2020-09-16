#include <iostream>
#include <vector>

#include "../safe-containers/safe_array.h"

using namespace std::chrono_literals;

template<class T>unsigned char identify(T&& v) {return v;}

// g++-10 -stdc=c++20 -pthread test/copy.cpp -o ~/bin/safety/test_copy
int main(int argc, char** argv)
{
    const int N = (argc > 1) ? std::stoi( argv[1] ) : 20;

    using SA = sa::SafeArray<int>;
    SA sa{N};

    // Initialize
    for (auto it=sa.begin(); it!=sa.end(); ++it)
        *it = (it - sa.begin());    

    auto begin = sa.begin();
    auto next = begin++;
    std::cout << *begin << ", " << *next << std::endl;
    std::cout << &begin << ", " << &next << ", " << (begin==next) << std::endl;

    auto sbegin = sa.begin();
    std::cout << *sbegin << std::endl;
    sbegin++;
    std::cout << *sbegin << std::endl;
//    auto snext = sbegin++;
//    std::cout << *sbegin << ", " << *snext << std::endl;
//    std::cout << &sbegin << ", " << &snext << ", " 
//        << (sbegin==snext) << std::endl;

    std::cout << "...and we're done." << std::endl;

    return 0;
}
