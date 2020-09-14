#ifndef SCOPETRACKER_H
#define SCOPETRACKER_H

#include <chrono>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

#include "log.h"

class ScopeTracker
{
    public:
        ScopeTracker( const std::string&& name,
                std::thread::id tid=std::this_thread::get_id() )
            : _name(name),
            _startTime(std::chrono::steady_clock::now()),
            _tab(MakeTab(tid)),
            _tid(tid),
            _log{ Log::instance() }
        {
            std::stringstream ss;
            ss << _tid;
            const std::string s = ss.str() + _tab + "Entered " + _name + "\n";
            _log.write(s);
            std::lock_guard<std::mutex> lock(_mutex);
            ++_tabCts[_tid];
        }

        ~ScopeTracker()
        {
            std::stringstream ss;
            ss << _tid;
            const std::string s = ss.str() + _tab + "Exited " 
                + _name + _add + " after " 
                + std::to_string(ElapsedMs()) + "ms\n";
            _log.write(s);
            std::lock_guard<std::mutex> lock(_mutex);
            --_tabCts[_tid];
        }

        static void InitThread(std::thread::id tid=std::this_thread::get_id())
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if ( !_tabCts.contains(tid) )
                _tabCts[tid] = 0;
        }

        void Add(const std::string& mesg) //For adding milestones
        {
            _add += ", " + mesg;
        }

        int ElapsedMs() const
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - _startTime).count();
        }
        

    private:
        inline std::string MakeTab(std::thread::id tid)
        {
            return std::string(1+_tabCts[tid]*4, ' ');
        }

        std::string _add;
        static std::mutex _mutex;
        const std::string _name;
        const std::chrono::steady_clock::time_point _startTime;
        static std::map<std::thread::id, int> _tabCts;
        const std::string _tab;
        const std::thread::id _tid;
        Log& _log;
};

std::mutex ScopeTracker::_mutex;
std::map<std::thread::id, int> ScopeTracker::_tabCts;

#endif // SCOPETRACKER_H
