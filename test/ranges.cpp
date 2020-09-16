#include <algorithm>
#include <vector>
#include <ranges>
#include <iostream>
 
#include "../safe-containers/safe_array.h"

template <typename T, size_t const Size>
class dummy_array 
{
    T data[Size] = {};
    public:
        T const & GetAt(size_t const index) const
        {
            if (index < Size) return data[index];
            throw std::out_of_range("index out of range");
        } 
        T& GetAt(size_t const index)
        {
            if (index < Size) return data[index];
            throw std::out_of_range("index out of range");
        } 

        void SetAt(size_t const index, T const & value)
        {
            if (index < Size) data[index] = value;
            else throw std::out_of_range("index out of range");
        }

        size_t GetSize() const { return Size; }
};

//class dummy_array_iterator;

template <typename T, typename C, size_t const Size>
class dummy_array_iterator_type
{
    public:
        typedef std::random_access_iterator_tag iterator_category;
        typedef dummy_array_iterator_type<T,C,Size> self_type;

//        friend dummy_array_iterator operator+<T>(size_t, dummy_array_iterator);
//        friend dummy_array_iterator operator-<T>(size_t, dummy_array_iterator);

        dummy_array_iterator_type(C& collection,
                size_t const index) :
            index(index), collection(collection)
        {}

        T& operator[](size_t index)
        {
            return collection.GetAt(index);
        }

        T const & operator* () const
        {
            return collection.GetAt(index);
        }
        bool operator==(dummy_array_iterator_type const & other) const
        {
            return index == other.index;
        }
        bool operator!=(dummy_array_iterator_type const & other) const
        {
            return !(*this == other);
        }
        bool operator<(dummy_array_iterator_type const & other) const
        {
            return collection.GetAt(index) < other.collection.GetAt(index);
        }
        bool operator>(dummy_array_iterator_type const & other) const
        {
            return collection.GetAt(index) > other.collection.GetAt(index);
        }
        bool operator>=(dummy_array_iterator_type const & other) const
        {
            return *this>other || *this==other;
        }
        bool operator<=(dummy_array_iterator_type const & other) const
        {
            return *this<other || *this==other;
        }

        self_type& operator+=(size_t n)
        {
            index += n;
            return *this;
        }
        self_type& operator-=(size_t n)
        {
            index -= n;
            return *this;
        }
        
        self_type& operator++ ()
        {
            ++index;
            return *this;
        }

//    private:
        size_t   index;
        C&       collection; 
};

template <typename T, size_t const Size>
using dummy_array_iterator
    = dummy_array_iterator_type<T, dummy_array<T, Size>, Size>; 

template <typename T, size_t const Size>
using dummy_array_const_iterator
    = dummy_array_iterator_type<T, dummy_array<T, Size> const, Size>;


template <typename T, size_t const Size>
dummy_array_iterator<T,Size> operator+(size_t n,
        const dummy_array_iterator<T,Size>& it)
{
    dummy_array_iterator<T,Size> out{it};
    out.index += n;
    return out;
}
template <typename T, size_t const Size>
dummy_array_iterator<T,Size> operator+(const dummy_array_iterator<T,Size>& it,
        size_t n)
{
    return n + it;
}
template <typename T, size_t const Size>
dummy_array_iterator<T,Size> operator-(size_t n,
        const dummy_array_iterator<T,Size>& it)
{
    dummy_array_iterator<T,Size> out{it};
    out.index -= n;
    return out;
}
template <typename T, size_t const Size>
dummy_array_iterator<T,Size> operator-(const dummy_array_iterator<T,Size>& it,
        size_t n)
{
    return n - it;
}


template <typename T, size_t const Size>
inline dummy_array_iterator<T, Size> begin(dummy_array<T, Size>& collection)
{
    return dummy_array_iterator<T, Size>(collection, 0);
} 

template <typename T, size_t const Size>
inline dummy_array_iterator<T, Size> end(dummy_array<T, Size>& collection)
{
    return dummy_array_iterator<T, Size>(collection, collection.GetSize());
} 

template <typename T, size_t const Size>
inline dummy_array_const_iterator<T, Size> begin(
        dummy_array<T, Size> const & collection)
{
    return dummy_array_const_iterator<T, Size>(collection, 0);
}

template <typename T, size_t const Size>
inline dummy_array_const_iterator<T, Size> end(
        dummy_array<T, Size> const & collection)
{
    return dummy_array_const_iterator<T, Size>(
            collection, collection.GetSize());
}

int main()
{
    auto even = [](int i){ return 0 == i % 2; };
    dummy_array<int,10> d;
//    int ct = 0;
//    for (auto& it : d)
//        it = ct++;
//    for (auto&& it : d | std::views::filter(even))
//        std::cout << it << ", ";
    std::cout << std::endl;

    std::ranges::sort(d);
/*
    constexpr int N = 10;
    using SA = sa::SafeArray<int>;
    SA ints{N};
    int ct = 0;
    for (auto& it : ints)
        it = N - ct++;

    for (auto& i : ints) { std::cout << i << ' '; }
    std::cout << std::endl;

    std::ranges::sort(ints);
    
    for (auto& i : ints) { std::cout << i << ' '; }
    std::cout << std::endl;
*/
/*    for ( int i : ints | std::views::filter(even) )
    {
        std::cout << i << ' ';
    }
    std::cout << std::endl;
*/
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
