#ifndef SAFE_ARRAY_H
#define SAFE_ARRAY_H

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "access_ctr.h"

#ifdef DEBUG_ACCESS
    #define FUNC_LOGGING() ScopeTracker st{__func__}
#else
    #define FUNC_LOGGING() 0
#endif

using namespace std::chrono_literals;

namespace sa
{

// Starting off modifying https://gist.github.com/jeetsukumaran/307264

template <typename T>
class SafeArray
{
    public:
        class SafeIterator;

        typedef int size_type;
        typedef std::atomic<int> count_type;
        typedef std::atomic<bool> flag_type;
        typedef std::shared_ptr<AccessCtr> AccessCtrPtr;
        typedef std::shared_ptr<std::condition_variable> CondVarPtr;
        typedef SafeIterator iterator;
        using thread_id = AccessCtr::thread_id;

        class Iterator
        {
            public:
                typedef Iterator self_type;
                typedef T value_type;
                typedef T& reference;
                typedef T* pointer;
                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;
                Iterator(pointer ptr) 
                    : _ptr(ptr),
                    _pause{50}
                {}
                self_type operator++(int)
                {
                    self_type i = *this;
                    _ptr++;
#ifdef DEMO
                    std::this_thread::sleep_for(
                            std::chrono::milliseconds(_pause) );
#endif
                    return i;
                }
                self_type operator++()
                {
                    _ptr++;
#ifdef DEMO
                    std::this_thread::sleep_for(
                            std::chrono::milliseconds(_pause) );
#endif
                    return *this;
                }
                reference operator*() { return *_ptr; }
                pointer operator->() { return _ptr; }
                difference_type operator-(const self_type& rhs)
                {
                    return _ptr - rhs._ptr;
                }
                bool operator==(const self_type& rhs)
                {
                    return _ptr == rhs._ptr;
                }
                bool operator!=(const self_type& rhs)
                {
                    return !(*this == rhs);
                }
            private:
                pointer _ptr;
                int _pause;
        };
        // We'll worry about const_iterator later
        
        class SafeIterator
        {
            public:
                enum class ITER_MODE
                {
                    UNKNOWN = -1,
                    READ,
                    READ_WRITE
                };

                typedef SafeIterator self_type;
                typedef T value_type;
                typedef T& reference;
                typedef T* pointer;
                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;

                SafeIterator(pointer ptr, CondVarPtr cond_var,
                        AccessCtrPtr access_ctr, std::mutex& mutex,
                        ITER_MODE iter_mode=ITER_MODE::READ) 
                    : _ptr(ptr),
                    _access_ctr{access_ctr},
                    _cond_var{cond_var},
                    _pause{50},
                    _iter_mode{iter_mode},
                    _mutex{&mutex}
                {
                    FUNC_LOGGING();
                    _access_ctr->add_thread();
                    _update_counters(1);
                }

                SafeIterator(const SafeIterator& rhs) 
                {
                    FUNC_LOGGING();
                    _copy_data(rhs);
                    _update_counters(1);
                }
                SafeIterator(SafeIterator&&) = delete;
                SafeIterator& operator=(const SafeIterator& rhs)
                {
                    FUNC_LOGGING();
                    _copy_data(rhs);
                    _update_counters(1);
                }
                SafeIterator& operator=(SafeIterator&&) = delete;
                ~SafeIterator()
                {
                    FUNC_LOGGING();
                    std::lock_guard<std::mutex> lock{ *_mutex };
                    _update_counters(-1);
                    _cond_var->notify_all();
#ifdef DEBUG_SAFE
                    const std::string s = "Total reader, writer cts: " 
                        + std::to_string(_access_ctr->get_all_reader_ct())
                        + ","
                        + std::to_string(_access_ctr->get_all_writer_ct());
                    st.add(s);
#endif
                }

                self_type operator++(int)
                {
                    self_type iter = *this;
                    _ptr++;
#ifdef DEMO
                    std::this_thread::sleep_for(_pause);
#endif
                    return iter;
                }

                self_type& operator++() 
                {
                    _ptr++;
#ifdef DEMO
                    std::this_thread::sleep_for(_pause);
#endif
                    return *this;
                }
                reference operator*() { return *_ptr; }
                pointer operator->() { return _ptr; }
                difference_type operator-(const self_type& rhs) const
                { return _ptr - rhs._ptr; }
                bool operator<(const self_type& rhs) const
                { return *_ptr < *rhs._ptr; }
                bool operator==(const self_type& rhs) const
                { return _ptr == rhs._ptr; }
                bool operator!=(const self_type& rhs) const
                { return !(*this == rhs); }

            private:
                void _copy_data(const SafeIterator& other)
                {
                    _ptr = other._ptr;
                    _access_ctr = other._access_ctr;
                    _cond_var = other._cond_var;
                    _pause = other._pause;
                    _iter_mode = other._iter_mode;
                    _mutex = other._mutex;
                }
                void _update_counters(int update)
                {
                    FUNC_LOGGING();
                    switch (_iter_mode)
                    {
                        case ITER_MODE::READ:
                            _access_ctr->reader_update(update);
                            break;
                        case ITER_MODE::READ_WRITE:
                            _access_ctr->reader_update(update);
                            _access_ctr->writer_update(update);
                            break;
                        default:
                            break;
                    }
#ifdef DEBUG_SAFE
                    const std::string s = "Current thd reader, writer cts: " 
                        + std::to_string(_access_ctr->get_all_reader_ct())
                        + ","
                        + std::to_string(_access_ctr->get_all_writer_ct());
                    st.add(s);
#endif
                }

                pointer _ptr;
                AccessCtrPtr _access_ctr;
                CondVarPtr _cond_var;
                std::chrono::milliseconds _pause;
                ITER_MODE _iter_mode;
                std::mutex* _mutex;
        };

        SafeArray(size_type size) 
            : _size(size),
            _access_ctr{ new AccessCtr() },
            _cond_var{ new std::condition_variable() },
            _data{ new T[size] }
        {
            FUNC_LOGGING();
        }

        SafeArray(const SafeArray&) = delete;
        SafeArray& operator=(const SafeArray&) = delete;

        ~SafeArray()
        {
            FUNC_LOGGING();
            delete[] _data;
        }

        size_type size() const { return _size; }

        // Random access without explict construction of an iterator is
        // read-only, sorry
        const T& operator[](size_type index) const
        {
            assert(index < _size);
            return _data[index];
        }

        Iterator unsafe_begin() { return Iterator(_data); }
        Iterator unsafe_end() { return Iterator(_data + _size); }

        SafeIterator cbegin() 
        {
            FUNC_LOGGING();
            return safe_read_iterator(0);
        }
        SafeIterator cend() 
        {
            FUNC_LOGGING();
            return safe_read_iterator(_size);
        }
        SafeIterator begin() 
        {
            FUNC_LOGGING();
            return safe_rw_iterator(0);
        }
        SafeIterator end() 
        {
            FUNC_LOGGING();
            return safe_rw_iterator(_size);
        }

        int get_writer_ct() const 
        {
            FUNC_LOGGING();
            return _access_ctr->get_writer_ct();
        }
        int get_reader_ct() const 
        {
            FUNC_LOGGING();
            return _access_ctr->get_reader_ct();
        }

    private:
        SafeIterator safe_rw_iterator(size_type offset)
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _cond_var->wait(lock, [this]{
                    return !_access_ctr->get_has_other_accessors();
                    });
            SafeIterator safe_iter = SafeIterator{_data+offset, _cond_var,
                _access_ctr, _mutex, SafeIterator::ITER_MODE::READ_WRITE};
            return safe_iter;
        }
        SafeIterator safe_read_iterator(size_type offset)
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _cond_var->wait(lock, [this]{
                    return !_access_ctr->get_has_other_writers();
                    });
            SafeIterator safe_iter = SafeIterator{_data+offset, _cond_var,
                _access_ctr, _mutex};
            return safe_iter;
        }

        T* _data;
        size_type _size;

        AccessCtrPtr _access_ctr;

        mutable CondVarPtr _cond_var;
        mutable std::mutex _mutex;
};

} // sa

/*
template<typename T>
sa::SafeArray<T>::SafeIterator begin(sa::SafeArray<T>& safe_array)
{
    return safe_array.begin();
}
template<typename T>
sa::SafeArray<T>::SafeIterator end(sa::SafeArray<T>& safe_array)
{
    return safe_array.end();
}
*/

#undef FUNC_LOGGING

#endif // SAFE_ARRAY_H
