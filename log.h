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

        void add(std::string const& mesg)
        {
            _buffer += mesg;
        }
        void write(std::string mesg="")
        {
            std::lock_guard<std::mutex> lock{_mutex};
            _ostm << _buffer << mesg << std::endl << std::flush;
            _buffer = "";
        }

    private:
        Log(std::ostream& ostm) : _ostm{ostm} {}

        static Log* _instance;
        std::string _buffer;
        std::ostream& _ostm;
        std::mutex _mutex;
};

Log* Log::_instance = nullptr;

#endif // LOG_H
