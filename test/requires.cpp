#include <iostream>

#include "../safe-containers/safe_array.h"

/*
#define define_has_member(member_name)                                         \
    template <typename T>                                                      \
    class has_member_##member_name                                             \
    {                                                                          \
        typedef char yes_type;                                                 \
        typedef long no_type;                                                  \
        template <typename U> static yes_type test(decltype(&U::member_name)); \
        template <typename U> static no_type  test(...);                       \
    public:                                                                    \
        static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes_type);  \
    }
*/

class C
{
    public:
        C& operator[](size_t index)
        {
            return *this;
        }
        void f() { std::cout << "Hooray" << std::endl; }
};

class D
{
    public:
/*        C& operator[](size_t index)
        {
            return *this;
        }
*/
};


/*
template<typename T>
class has_nc_op
{
    using V = decltype(&T::operator[]);
    public:
        static constexpr int value = sizeof(V);
//        static constexpr bool value = sizeof(
};
*/
/*
template <typename T, typename = void>
struct has_nc_op : std::false_type{};

template <typename T>
struct has_nc_op<T, decltype((void)T::member, void())> : std::true_type {};
*/

template <typename T, typename = void>
struct has_nc_op : std::false_type {};

template <typename T>
struct has_nc_op<T, decltype(&T::f, void())> : std::true_type {};
//struct has_nc_op<T, decltype((void)T::f, void())> : std::true_type {};

struct yay : std::true_type {};

int main(void)
{
    has_nc_op<C> valC;
    has_nc_op<D> valD;
    decltype(&C::f) v; 
    std::cout << valC << ", " << valD << std::endl;
    std::cout << has_nc_op<C>::value << std::endl;
    std::cout << has_nc_op<D>::value << std::endl;
    return 0;
}
