#ifndef SAFE_ARRAY_V2_H
#define SAFE_ARRAY_V2_H

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
namespace v2
{

    template <typename T>
    class SafeArray
    {
    public:
        class SafeIterator;

        typedef int size_type;
        typedef std::atomic<int> count_type;
        typedef std::atomic<bool> flag_type;
        typedef T value_type;
        typedef std::shared_ptr<AccessCtr> AccessCtrPtr;
        typedef std::shared_ptr<std::condition_variable> CondVarPtr;
        typedef SafeIterator iterator;
        using thread_id = AccessCtr::thread_id;

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

        class Iterator
        {
            public:
                typedef Iterator self_type;
                typedef T value_type;
                typedef T& reference;
                typedef T* pointer;
                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;
                Iterator(size_type pos, thread_id tid)
                    : _pos{pos}, _tid{tid}
                {}
                Iterator(const Iterator&) = delete;
                Iterator(Iterator&&) = delete;
                Iterator& operator=(const Iterator&) = delete;
                Iterator& operator=(Iterator&&) = delete;
                ~Iterator() 
                {
                    FUNC_LOGGING();
                }

                self_type operator++(int)
                {
                    self_type iter = *this;
                    _ptr++;
                    return iter;
                }

                self_type& operator++()
                {
                    _ptr++;
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
                size_type _pos;
                const thread_id _tid;
        };

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
}
    
}

#endif // SAFE_ARRAY_V2
