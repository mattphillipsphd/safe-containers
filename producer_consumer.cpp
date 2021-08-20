// A simple example of producer/consumer code that doesn't use while(1), 
// while(true) etc. loops

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>


std::atomic<bool> data_available{false};

class Consumer
{
    public:
        static void run(Consumer& consumer)
        {
            consumer();
        }

        Consumer() : _is_done{false}
        {
            std::cout << "Consumer::Consumer()" << std::endl;
        }

        void operator()()
        {
            std::cout << "Consumer::operator()" << std::endl;
            while ( !get_is_done() )
            {
                std::unique_lock<std::mutex> lock{_mutex};
                while ( !_messages.empty() )
                {
                    std::cout << "Consumed: " << _messages.front() << ", " 
                        << "thread id " << std::this_thread::get_id() << ", "
                        << _messages.size() - 1 << " messages remaining" 
                        << std::endl;
                    _messages.pop();
                }
                data_available = false;

                _cond_var.wait(lock, []{ return data_available.load(); });
                std::this_thread::sleep_for( std::chrono::milliseconds(200) );
            }
            std::cout << "Consumer::operator() finished" << std::endl;
        }

        void add_message(const std::string& message)
        {
            std::cout << "Consumer::add_message" << std::endl;
            std::unique_lock<std::mutex> lock{_mutex};
            _messages.push(message);
            data_available = true;
            _cond_var.notify_all();
        }

        bool get_is_done() const
        {
            std::unique_lock<std::mutex> lock{_mutex};
            return _is_done;
        }

        void set_is_done(bool is_done)
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _is_done = is_done;
        }

    private:
        bool _is_done;
        std::queue<std::string> _messages;
        mutable std::mutex _mutex;
        std::condition_variable _cond_var;
};

class Producer
{
    public:
        Producer(Consumer& consumer) :
            _consumer{consumer}
        {
            std::cout << "Producer::Producer()" << std::endl;
        }

        void operator()()
        {
            std::cout << "Producer::operator()" << std::endl;
            _consumer.set_is_done(false);
            for (int i=0; i<20; ++i)
            {
                const std::string s{ "Message #" + std::to_string(i) };
                std::cout << "Produced: " << s << ", "
                        << "thread id " << std::this_thread::get_id()
                        << std::endl;
                _consumer.add_message(s);
                std::this_thread::sleep_for( std::chrono::milliseconds(200) );
            }
            _consumer.set_is_done(true);
            _consumer.add_message("Final message");
            std::cout << "Producer::operator() finished" << std::endl;
        }

    private:
        Consumer& _consumer;
};

int main(int, char**)
{
    Consumer consumer;
    Producer producer{consumer};
    std::thread t1{producer};
    std::thread t2{ &Consumer::run, std::ref(consumer) };
    t1.detach();
    t2.join();
    return 0;
}
