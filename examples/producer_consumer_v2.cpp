/* A simple standalone example of producer/consumer code that doesn't use 
 * while(1), while(true) etc. loops.  The key idea though is the use of a 
 * condition variable.
 *
 * This version also creates a separate 'Store' class for the data, and all the
 * synchronization is restricted to this class, leading to (what seems like)
 * a better, simple architecture overall.
*/

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

using VecStr = std::vector<std::string>;

class MessageStore
{
    public:
        static MessageStore& instance()
        {
            if (!_instance) _instance = new MessageStore{};
            return *_instance;
        }

        void add_message(const std::string& message)
        {
            std::unique_lock<std::mutex> lock{_mutex};
            std::cout << "MessageStore::add_message" << std::endl;
            _messages.push(message);
            _data_available = true;
            _cond_var.notify_all();
            std::this_thread::sleep_for( std::chrono::milliseconds(200) );
        }

        VecStr consume_messages()
        {
            std::unique_lock<std::mutex> lock{_mutex};
            VecStr vec_str;
            while ( !_messages.empty() )
            {
                vec_str.push_back( _messages.front() );
                _messages.pop();
            }
            _data_available = false;
            return vec_str;
        }

        void wait_for_messages()
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _cond_var.wait(lock, [this]{ return _data_available.load(); });
            std::this_thread::sleep_for( std::chrono::milliseconds(200) );
        }

        bool get_is_all_done() const
        {
            std::unique_lock<std::mutex> lock{_mutex};
            return _is_all_done;
        }

        void set_is_all_done(bool is_all_done)
        {
            std::unique_lock<std::mutex> lock{_mutex};
            _is_all_done = is_all_done;
        }

    private:
        MessageStore() : _data_available{false}
        {}
        MessageStore(MessageStore&) = delete;
        MessageStore(MessageStore&&) = delete;

        static MessageStore* _instance;

        std::atomic<bool> _data_available;
        bool _is_all_done;
        std::queue<std::string> _messages;
        mutable std::mutex _mutex;
        std::condition_variable _cond_var;
};
MessageStore* MessageStore::_instance{nullptr};

class Consumer
{
    public:
        Consumer() : 
            _msg_store{ MessageStore::instance() }
        {
            std::cout << "Consumer::Consumer()" << std::endl;
        }

        void operator()()
        {
            std::cout << "Consumer::operator()" << std::endl;
            while ( !_msg_store.get_is_all_done() )
            {
                const auto messages = _msg_store.consume_messages();
                for (const auto it : messages)
                    std::cout << "Consumed: " << it << ", " << "thread id "
                        << std::this_thread::get_id() << std::endl;

                _msg_store.wait_for_messages();
            }
            std::cout << "Consumer::operator() finished" << std::endl;
        }

    private:
        MessageStore& _msg_store;
};

class Producer
{
    public:
        Producer() :
            _msg_store{ MessageStore::instance() }
        {
            std::cout << "Producer::Producer()" << std::endl;
        }

        void operator()()
        {
            std::cout << "Producer::operator()" << std::endl;
            _msg_store.set_is_all_done(false);
            for (int i=0; i<20; ++i)
            {
                const std::string s{ "Message #" + std::to_string(i) };
                std::cout << "Produced: " << s << ", "
                        << "thread id " << std::this_thread::get_id()
                        << std::endl;
                _msg_store.add_message(s);
            }
            _msg_store.set_is_all_done(true);
            _msg_store.add_message("Final message");
            std::cout << "Producer::operator() finished" << std::endl;
        }

    private:
        MessageStore& _msg_store;
};

int main(int, char**)
{
    std::thread t1{ Producer{} };
    std::thread t2{ Consumer{} };
    t1.detach();
    t2.join();
    return 0;
}
