#ifndef SAFE_ARRAY_H
#define SAFE_ARRAY_H

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "access_ctr.h"

using namespace std::chrono_literals;

template <typename T>
class SafeArray
{
    public:

        typedef int size_type;
        typedef std::atomic<int> count_type;
        typedef std::atomic<bool> flag_type;
        using thread_id = AccessCtr::thread_id;

        class iterator
        {
            public:
                typedef iterator self_type;
                typedef T value_type;
                typedef T& reference;
                typedef T* pointer;
                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;
                iterator(pointer ptr) 
                    : _ptr(ptr),
                    _pause{50}
                {}
                self_type operator++(int)
                {
                    self_type i = *this;
                    _ptr++;
                    std::this_thread::sleep_for(
                            std::chrono::milliseconds(_pause) );
                    return i;
                }
                self_type operator++()
                {
                    _ptr++;
                    std::this_thread::sleep_for(
                            std::chrono::milliseconds(_pause) );
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
        
        class safe_iterator
        {
            public:
                enum class ITER_MODE
                {
                    UNKNOWN = -1,
                    READ,
                    READ_WRITE
                };

                typedef safe_iterator self_type;
                typedef T value_type;
                typedef T& reference;
                typedef T* pointer;
                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;

                safe_iterator(pointer ptr, std::condition_variable& writer_cv,
                        std::shared_ptr<AccessCtr> access_ctr,
                        ITER_MODE iter_mode=ITER_MODE::READ) 
                    : _ptr(ptr),
                    _access_ctr{access_ctr},
                    _writer_cv{&writer_cv},
                    _pause{50},
                    _iter_mode{iter_mode}
                {
#ifdef DEBUG_SAFE
                    ScopeTracker st{"safe_iterator::safe_iterator"};
#endif
                    _access_ctr->add_thread();
                    _update_counters(1);
                }

                safe_iterator(const safe_iterator& rhs) 
                {
#ifdef DEBUG_SAFE
                    ScopeTracker st{"safe_iterator::<copy ctor>"};
#endif
                    _copy_data(rhs);
                    _update_counters(1);
                }
                safe_iterator(safe_iterator&&) = default;
                safe_iterator& operator=(const safe_iterator& rhs)
                {
#ifdef DEBUG_SAFE
                    ScopeTracker st{"safe_iterator::operator="};
#endif
                    _copy_data(rhs);
                    _update_counters(1);
                }
                safe_iterator& operator=(safe_iterator&&) = default;
                ~safe_iterator()
                {
#ifdef DEBUG_SAFE
                    ScopeTracker st{"safe_iterator::~safe_iterator"};
#endif
                    _update_counters(-1);
                    _writer_cv->notify_all();
                }

                self_type operator++(int)
                {
                    self_type iter = *this;
                    _ptr++;
                    std::this_thread::sleep_for(_pause);
                    return iter;
                }

                self_type& operator++() 
                {
                    _ptr++;
                    std::this_thread::sleep_for(_pause);
                    return *this;
                }
                reference operator*() 
                {
                    return *_ptr;
                }
                pointer operator->() 
                {
                    return _ptr;
                }
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
                void _copy_data(const safe_iterator& other)
                {
                    _ptr = other._ptr;
                    _access_ctr = other._access_ctr;
                    _writer_cv = other._writer_cv;
                    _pause = other._pause;
                    _iter_mode = other._iter_mode;
                }
                void _update_counters(int update)
                {
#ifdef DEBUG_SAFE
                    ScopeTracker st{"safe_iterator::_update_counters"};
#endif
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
                    const std::string s = "Reader ct: " 
                        + std::to_string(_access_ctr->get_reader_ct())
                        + ", writer ct: "
                        + std::to_string(_access_ctr->get_writer_ct());
                    st.Add(s);
#endif
                }

                pointer _ptr;
                std::shared_ptr<AccessCtr> _access_ctr;
                std::condition_variable* _writer_cv;
                std::chrono::milliseconds _pause;
                ITER_MODE _iter_mode;
        };

        SafeArray(size_type size) 
            : _size(size),
            _access_ctr{ new AccessCtr() },
            _data{ new T[size] }
        {
#ifdef DEBUG_SAFE
            ScopeTracker st{"SafeArray"};
#endif
        }

        ~SafeArray()
        {
#ifdef DEBUG_SAFE
            ScopeTracker st{"~SafeArray"};
#endif
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

        iterator unsafe_begin() { return iterator(_data); }
        iterator unsafe_end() { return iterator(_data + _size); }

        safe_iterator cbegin() 
        {
#ifdef DEBUG_SAFE
            ScopeTracker::InitThread();
            ScopeTracker st{"cbegin"};
#endif
            return safe_read_iterator(0);
        }
        safe_iterator cend() 
        {
#ifdef DEBUG_SAFE
            ScopeTracker st{"cend"};
#endif
            return safe_read_iterator(_size);
        }
        safe_iterator begin() 
        {
#ifdef DEBUG_SAFE
            ScopeTracker::InitThread();
            ScopeTracker st{"begin"};
#endif
            return safe_rw_iterator(0);
        }
        safe_iterator end() 
        {
#ifdef DEBUG_SAFE
            ScopeTracker st{"end"};
#endif
            return safe_rw_iterator(_size);
        }

        int get_writer_ct() const 
        {
#ifdef DEBUG_SAFE
            ScopeTracker st{"get_writer_ct"};
#endif
            return _access_ctr->get_writer_ct();
        }
        int get_reader_ct() const 
        {
#ifdef DEBUG_SAFE
            ScopeTracker st{"get_reader_ct"};
#endif
            return _access_ctr->get_reader_ct();
        }

    private:
        safe_iterator safe_rw_iterator(size_type offset)
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _cond_var.wait(lock, [this]{
                    return !_access_ctr->get_has_other_writers()
                            && !_access_ctr->get_has_other_readers();
                    });
            safe_iterator safe_iter = safe_iterator{_data+offset, _cond_var,
                _access_ctr, safe_iterator::ITER_MODE::READ_WRITE};
            return safe_iter;
        }
        safe_iterator safe_read_iterator(size_type offset)
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _cond_var.wait(lock, [this]{
                    return !_access_ctr->get_has_other_writers();
                    });
            safe_iterator safe_iter = safe_iterator{_data+offset, _cond_var,
                _access_ctr};
            return safe_iter;
        }

        T* _data;
        size_type _size;

        std::shared_ptr<AccessCtr> _access_ctr;

        mutable std::condition_variable _cond_var;
        mutable std::mutex _mutex;
};

#endif // SAFE_ARRAY_H
