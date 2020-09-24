#include <iostream>
#include <type_traits>

#include "../safe-containers/safe_array.h"

template<class T>
decltype(auto) identify(T&& v); 

class B
{
    public:
        typedef int value_type;
        B() 
        {
            _d = new int[10]; 
            for (int i=0; i<10; ++i)
                _d[i] = i;
        }
        B(const B& other)
        {
            _d = new int[10]; 
            for (int i=0; i<10; ++i)
                _d[i] = other._d[i];
        }
        ~B() { delete[] _d; }
    protected:
        int* _d;
};

class C : public B
{
    public:
        int& operator[](size_t index)
        {
            return _d[index];
        }
        void f() { std::cout << "Hooray" << std::endl; }
};

class D : public B
{
    public:
        const int& operator[](size_t index) const
        {
            return _d[index];
        }
};


template <typename T, typename = void>
struct has_nc_op : std::false_type
{
};

/*
template <typename T>
struct has_nc_op<T, decltype(&T::operator[], void())> : std::true_type 
{
};
*/

template <typename T>
struct has_nc_op<T, decltype(&T::operator[], void())> : std::true_type 
{
};

template<typename T>
concept RandomWrite = has_nc_op<T>::value \
                      && std::is_same<decltype(&T::operator[]),
        typename T::value_type& (T::*)(size_t)>::value;

int first(RandomWrite auto cont)
{
    return cont[4];
}

template<typename T>
auto h(T a) -> double
{
    return a*a;
}

int main(void)
{
    /*
    has_nc_op<C> valC;
    has_nc_op<D> valD;
    std::cout << valC << ", " << valD << std::endl;
    std::cout << has_nc_op<C>::value << std::endl;
    std::cout << has_nc_op<D>::value << std::endl;
    */

    C c;
    D d;
    std::cout << first(c) << std::endl;
//    std::cout << first(d) << std::endl;

    decltype(&C::operator[]) m;
    int& (C::* j)(size_t);
//    identify(j);
    std::cout << h(6.3) << std::endl;

    sa::SafeArray<int> s{50};
    return 0;
}
