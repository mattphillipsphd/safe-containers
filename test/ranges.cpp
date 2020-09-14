#include <vector>
#include <ranges>
#include <iostream>
 
#include "../safe-containers/safe_array.h"

int main()
{
    constexpr int N = 10;
    using SA = SafeArray<int>;
    SA ints{N};
    int ct = 0;
    for (auto& i : ints)
        i = ct++;
    
    for (auto& i : ints)
    {
        std::cout << i << ' ';
    }
    std::cout << std::endl;

    auto even = [](int i){ return 0 == i % 2; };
    for ( int i : ints | std::views::filter(even) )
    {
        std::cout << i << ' ';
    }
    std::cout << std::endl;
/*    auto square = [](int i) { return i * i; };
 
    for (int i : ints | std::views::filter(even) 
            | std::views::transform(square))
    {
        std::cout << i << ' ';
    }
    std::cout << std::endl;
*/
    return 0;
}
