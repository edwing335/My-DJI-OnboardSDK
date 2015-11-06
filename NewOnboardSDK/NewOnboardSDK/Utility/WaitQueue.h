#ifndef WAITQUEUE_H_INCLUDED
#define WAITQUEUE_H_INCLUDED

#include <queue>
#include <utility>
#include <mutex>
#include <condition_variable> 

template<typename Data>
class WaitQueue
{
public:
    void push(Data const& data)
    {
		{
			std::lock_guard<std::mutex> lock(the_mutex);
			the_queue.push(std::move(data));
		}
        the_condition_variable.notify_one();
    }

    bool empty() const
    {
		std::lock_guard<std::mutex> lock(the_mutex);
		return the_queue.empty();
    }

    bool try_pop(Data& popped_value)
    {
		std::lock_guard<std::mutex> lock(the_mutex);
		if (the_queue.empty())
		{
            return false;
        }

        popped_value=the_queue.front();
        the_queue.pop();
        return true;
    }

    void wait_and_pop(Data& popped_value)
    {
		std::unique_lock<std::mutex> lock(the_mutex);
		while (the_queue.empty())
        {
            the_condition_variable.wait(lock);
        }

        popped_value=the_queue.front();
        the_queue.pop();
        lock.unlock(); 
    }

private:
    std::queue<Data> the_queue;
    mutable std::mutex the_mutex;
    std::condition_variable the_condition_variable;

};

#endif // WAITQUEUE_H_INCLUDED
