#ifndef ACCESS_CTR_H
#define ACCESS_CTR_H

#include <cassert>
#include <mutex>
#include <ranges>
#include <unordered_map>

#include "../scopetracker.h"

#ifdef DEBUG_ACCESS
    #define FUNC_LOGGING() ScopeTracker st{__func__}
#else
    #define FUNC_LOGGING() 0
#endif


class AccessCtr
{
    public:
        using thread_id = std::thread::id;
        AccessCtr() = default;
        ~AccessCtr()
        {
            FUNC_LOGGING();
        }

        void add_thread( thread_id tid=std::this_thread::get_id() ) 
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            if ( !_dict.contains(tid) )
                _dict.emplace( std::make_pair(tid, Counters()) );
#ifdef DEBUG_ACCESS
            st.add(std::to_string(_dict.size())+" threads in Access Counter");
#endif
        }
        void remove_thread( thread_id tid=std::this_thread::get_id() )
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            assert( _dict.contains(tid) );
            _dict.erase(tid);
        }

        void reader_update( int update,
                thread_id tid=std::this_thread::get_id() )
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            _dict.at(tid).reader_ct += update;
        }
        void writer_update( int update,
                thread_id tid=std::this_thread::get_id() )
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            _dict.at(tid).writer_ct += update;
        }

        int get_all_reader_ct( thread_id tid=std::this_thread::get_id() ) const
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            int all_reader_ct{0};
            for (auto&& it : _dict | std::views::values)
                all_reader_ct += it.reader_ct;
            return all_reader_ct;
        }
        int get_all_writer_ct( thread_id tid=std::this_thread::get_id() ) const
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            int all_writer_ct{0};
            for (auto&& it : _dict | std::views::values)
                all_writer_ct += it.writer_ct;
            return all_writer_ct;
        }
        int get_reader_ct( thread_id tid=std::this_thread::get_id() ) const
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            if ( !_dict.contains(tid) )
                return 0;
            return _dict.at(tid).reader_ct;
        }
        int get_writer_ct( thread_id tid=std::this_thread::get_id() ) const
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            if ( !_dict.contains(tid) )
                return 0;
            return _dict.at(tid).writer_ct;
        }

        // We search for writers present in *other* threads.
        bool get_has_other_writers( thread_id tid=std::this_thread::get_id() )
            const
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            auto o_key = [tid](std::pair<thread_id,Counters> p)
            {
                return p.first != tid;
            };
            auto positive = [](std::pair<thread_id,Counters> p)
            {
                return p.second.writer_ct > 0;
            };
            for (auto&& it : _dict 
                    | std::views::filter(o_key) | std::views::filter(positive))
                return true;
            return false;
        }
        //
        // We search for accessors present in *other* threads.
        bool get_has_other_accessors(thread_id tid=std::this_thread::get_id())
            const
        {
            FUNC_LOGGING();
            std::lock_guard<std::mutex> lock(_mutex);
            auto o_key = [tid](std::pair<thread_id,Counters> p)
            {
                return p.first != tid;
            };
            auto positive = [](std::pair<thread_id,Counters> p)
            {
                return p.second.reader_ct > 0 || p.second.writer_ct > 0;
            };
            for (auto&& it : _dict 
                    | std::views::filter(o_key) | std::views::filter(positive))
                return true;
            return false;
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

#undef FUNC_LOGGING

#endif // ACCESS_CTR_H
