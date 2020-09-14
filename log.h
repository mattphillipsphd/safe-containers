#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <mutex>

class Log
{
    public:
        static Log& instance(std::ostream& ostm=std::cout)
        {
            if (!_instance)
                _instance = new Log(ostm);
            return *_instance;
        }

/*        std::ostream& operator<<(std::ostream& mesg)
        {
            std::lock_guard<std::mutex> lock{_mutex};
            return _ostm << mesg;
        }

        std::ostream& operator<<(std::string const& mesg)
        {
            std::lock_guard<std::mutex> lock{_mutex};
            return _ostm << mesg;
        }
*/
        void write(std::string const& mesg)
        {
            std::lock_guard<std::mutex> lock{_mutex};
            _ostm << mesg << std::endl;
        }

    private:
        Log(std::ostream& ostm) : _ostm{ostm} {}

        static Log* _instance;
        std::ostream& _ostm;
        std::mutex _mutex;
};

Log* Log::_instance = nullptr;

#endif // LOG_H
