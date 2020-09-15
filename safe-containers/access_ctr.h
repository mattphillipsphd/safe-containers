#ifndef ACCESS_CTR_H
#define ACCESS_CTR_H

#include <cassert>
#include <mutex>
#include <ranges>
#include <unordered_map>

#include "../scopetracker.h"

class AccessCtr
{
    public:
        using thread_id = std::thread::id;
        AccessCtr() = default;
        ~AccessCtr()
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"~AccessCtr"};
#endif
        }

        void add_thread( thread_id tid=std::this_thread::get_id() ) 
        {
#ifdef DEBUG_ACCESS
            ScopeTracker::InitThread();
            ScopeTracker st{"add_thread"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            if ( !_dict.contains(tid) )
                _dict.emplace( std::make_pair(tid, Counters()) );
#ifdef DEBUG_ACCESS
            st.Add(std::to_string(_dict.size())+" threads in Access Counter");
#endif
        }
        void remove_thread( thread_id tid=std::this_thread::get_id() )
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"remove_thread"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            assert( _dict.contains(tid) );
            _dict.erase(tid);
        }

        void reader_update( int update,
                thread_id tid=std::this_thread::get_id() )
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"reader_update"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            _dict.at(tid).reader_ct += update;
        }
        void writer_update( int update,
                thread_id tid=std::this_thread::get_id() )
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"writer_update"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            _dict.at(tid).writer_ct += update;
        }

        int get_all_reader_ct( thread_id tid=std::this_thread::get_id() ) const
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"get_all_reader_ct"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            int all_reader_ct{0};
            for (auto&& it : _dict | std::views::values)
                all_reader_ct += it.reader_ct;
            return all_reader_ct;
        }
        int get_all_writer_ct( thread_id tid=std::this_thread::get_id() ) const
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"get_all_writer_ct"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            int all_writer_ct{0};
            for (auto&& it : _dict | std::views::values)
                all_writer_ct += it.writer_ct;
            return all_writer_ct;
        }
        int get_reader_ct( thread_id tid=std::this_thread::get_id() ) const
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"get_reader_ct"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            if ( !_dict.contains(tid) )
                return 0;
            return _dict.at(tid).reader_ct;
        }
        int get_writer_ct( thread_id tid=std::this_thread::get_id() ) const
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"get_writer_ct"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            if ( !_dict.contains(tid) )
                return 0;
            return _dict.at(tid).writer_ct;
        }
        bool get_has_other_writers( thread_id tid=std::this_thread::get_id() )
            const
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"get_has_other_writers"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            bool has_other_writers{false};
            for (const auto& [key, value] : _dict)
            {
                if (key == tid)
                    continue;
                if (value.writer_ct > 0)
                {
                    has_other_writers = true;
                    break;
                }
            }
            return has_other_writers;
        }
        bool get_has_other_accessors(thread_id tid=std::this_thread::get_id())
            const
        {
#ifdef DEBUG_ACCESS
            ScopeTracker st{"get_has_other_accessors"};
#endif
            std::lock_guard<std::mutex> lock(_mutex);
            bool has_other_accessors{false};
            for (const auto& [key, value] : _dict)
            {
                if (key == tid)
                    continue;
                if (value.reader_ct > 0 || value.writer_ct > 0)
                {
                    has_other_accessors = true;
                    break;
                }
            }
            return has_other_accessors;
        }

    private:
        struct Counters
        {
            Counters() : reader_ct{0}, writer_ct{0} {}
            int reader_ct;
            int writer_ct;
        };

        std::unordered_map<thread_id, Counters> _dict;
        mutable std::mutex _mutex;
};

#endif // ACCESS_CTR_H
