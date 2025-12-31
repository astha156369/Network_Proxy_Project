#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/**
 * @file thread_pool.h
 * @brief Header for the ThreadPool class.
 * * Implements a thread-safe worker pool to manage concurrent client
 * connections without the overhead of creating new threads for every request.
 */

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

/**
 * @class ThreadPool
 * @brief A fixed-size pool of worker threads.
 * * This class maintains a set of worker threads and a task queue. It prevents
 * system exhaustion by reusing a limited number of threads to process tasks.
 */
class ThreadPool
{
public:
    /**
     * @brief Constructor that initializes the worker threads.
     * @param threads The number of worker threads to create in the pool.
     */
    ThreadPool(size_t threads);

    /**
     * @brief Adds a new task to the internal work queue.
     * @param task A function or lambda (usually the client handler) to be executed.
     */
    void enqueue(std::function<void()> task);

    /**
     * @brief Destructor that ensures all threads finish gracefully.
     * * Signals all threads to stop and joins them before the object is destroyed.
     */
    ~ThreadPool();

private:
    std::vector<std::thread> workers;        ///< Storage for the worker threads.
    std::queue<std::function<void()>> tasks; ///< Queue of pending tasks/connections.

    std::mutex queue_mutex;            ///< Mutex to protect access to the task queue.
    std::condition_variable condition; ///< Notifies threads when new tasks are available.
    bool stop;                         ///< Flag to signal the pool to shut down.
};

#endif 