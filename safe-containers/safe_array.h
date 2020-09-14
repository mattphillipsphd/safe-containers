#ifndef SAFE_ARRAY_H
#define SAFE_ARRAY_H

#include <atomic>
#include <cassert>
#include <condition_variable>
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
                self_type operator++()
                {
                    self_type i = *this;
                    _ptr++;
                    std::this_thread::sleep_for(
                            std::chrono::milliseconds(_pause) );
                    return i;
                }
                self_type operator++(int)
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
                    return _ptr != rhs._ptr;
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
                        AccessCtr& access_ctr,
                        ITER_MODE iter_mode=ITER_MODE::READ) 
                    : _ptr(ptr),
                    _access_ctr{&access_ctr},
                    _writer_cv{&writer_cv},
                    _pause{50},
                    _iter_mode{iter_mode}
                {
                    _access_ctr->add_thread();
                    _update_counters();
                    std::cout << "safe_iterator::safe_iterator" << std::endl;
                }

                safe_iterator(const safe_iterator& rhs) 
                {
                    _copy_data(rhs);
                    _update_counters();
                }
                safe_iterator(safe_iterator&&) = default;
                safe_iterator& operator=(const safe_iterator& rhs)
                {
                    _copy_data(rhs);
                    _update_counters();
                }
                safe_iterator& operator=(safe_iterator&&) = default;
                ~safe_iterator()
                {
                    switch (_iter_mode)
                    {
                        case ITER_MODE::READ:
                            _access_ctr->reader_sub();
                            break;
                        case ITER_MODE::READ_WRITE:
                            _access_ctr->reader_sub();
                            _access_ctr->writer_sub();
                            break;
                        default:
                            break;
                    }
                    std::string const s = "safe_iterator::~safe_iterator, "
                        "_reader_ct: " 
                        + std::to_string( _access_ctr->get_reader_ct() );
                    std::cout << s << std::endl;
                    _writer_cv->notify_all();
                }

                // Look at cpp20 no-op overloads, there is a way around this
                // TODO TODO This creates new iterators constantly
                self_type operator++(int)
                {
                    self_type iter{_ptr, *_writer_cv, *_access_ctr};
                        // Essentially have to copy by hand since copy ctor
                        // deleted
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
                    return _ptr != rhs._ptr;
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
                void _update_counters()
                {
                    switch (_iter_mode)
                    {
                        case ITER_MODE::READ:
                            _access_ctr->reader_add();
                            break;
                        case ITER_MODE::READ_WRITE:
                            _access_ctr->reader_add();
                            _access_ctr->writer_add();
                            break;
                        default:
                            break;
                    }
                }
                pointer _ptr;

                AccessCtr* _access_ctr;
                std::condition_variable* _writer_cv;
                std::chrono::milliseconds _pause;
                ITER_MODE _iter_mode;
        };

        SafeArray(size_type size) 
            : _size(size),
            _data{ new T[size] }
        {}

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

        safe_iterator cbegin() { return safe_read_iterator(0); }
        safe_iterator cend() { return safe_read_iterator(_size); }
        safe_iterator begin() { return safe_rw_iterator(0); }
        safe_iterator end() { return safe_rw_iterator(_size); }

        int get_writer_ct() const 
        {
            const thread_id tid{ std::this_thread::get_id() };
            return _access_ctr.get_writer_ct(tid);
        }
        int get_reader_ct() const 
        {
            const thread_id tid{ std::this_thread::get_id() };
            return _access_ctr.get_reader_ct(tid);
        }

    private:
        safe_iterator safe_rw_iterator(size_type offset)
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _cond_var.wait(lock, [this]{
                    return !_access_ctr.get_has_other_writers()
                            && !_access_ctr.get_has_other_readers();
                    });
            safe_iterator safe_iter = safe_iterator{_data+offset, _cond_var,
                _access_ctr, safe_iterator::ITER_MODE::READ_WRITE};
            return safe_iter;
        }
        safe_iterator safe_read_iterator(size_type offset)
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _cond_var.wait(lock, [this]{
                    return !_access_ctr.get_has_other_writers();
                    });
            safe_iterator safe_iter = safe_iterator{_data+offset, _cond_var,
                _access_ctr};
            return safe_iter;
        }

        T* _data;
        size_type _size;

        AccessCtr _access_ctr;

        mutable std::condition_variable _cond_var;
        mutable std::mutex _mutex;
};

#endif // SAFE_ARRAY_H
