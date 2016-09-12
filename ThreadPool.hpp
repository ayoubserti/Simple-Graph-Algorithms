//
//  ThreadPool.h
//  SimpleGraph
//
//  Created by Abderrahmane on 12/09/2016.
//  Inpired by the answer on http://stackoverflow.com/a/29742586/5500549
//

#ifndef ThreadPool_h
#define ThreadPool_h


#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <chrono>


class ThreadPool
{
    
    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    std::queue <std::function <void (void)>> jobs_;
    std::vector <std::thread> threads_;
    int size_;
    
public:
    
    ThreadPool (int threads) : shutdown_ (false)
    {
        // Create the specified number of threads
        for (int i = 0; i < threads; ++i)
            threads_.push_back(std::thread(std::bind (&ThreadPool::threadEntry, this, i)));
    }
    
    ~ThreadPool ()
    {
        {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);
            
            shutdown_ = true;
            condVar_.notify_all();
        }
        
        // Wait for all threads to stop
        std::cerr << "Joining threads" << std::endl;
        for (auto& thread : threads_)
            thread.join();
    }
    
    void doJob (std::function <void (void)> func)
    {
        // Place a job on the queue and unblock a thread
        std::unique_lock <std::mutex> l (lock_);
        ++size_;
        jobs_.push(func);
        condVar_.notify_one();
    }
    
    int size () { return size_; }
    
    void threadEntry (int i)
    {
        std::function <void (void)> job;
        
        while (1)
        {
            {
                std::unique_lock <std::mutex> l (lock_);
                
                while (! shutdown_ && jobs_.empty())
                    condVar_.wait (l);
                
                if (jobs_.empty ())
                {
                    // No jobs to do and we are shutting down
                    std::cerr << "Thread " << i << " terminates" << std::endl;
                    return;
                }
                
                std::cerr << "Thread " << i << " does a job" << std::endl;
                job = jobs_.front ();
                jobs_.pop();
            }
            
            // do the job
            job ();
            
            // decrement the size of waiting jobs
            {
             std::unique_lock <std::mutex> l (lock_);
                --size_;
            }
        }
        
    }
    
};
#endif /* ThreadPool_h */
