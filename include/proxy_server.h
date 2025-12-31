#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <winsock2.h>
#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ProxyServer
{
public:
    ProxyServer(int port);
    ~ProxyServer();

    void start();
    void stop();

private:
    void handle_client(SOCKET clientSocket);
    void worker_thread();

    int m_port;
    SOCKET m_listenSocket;
    std::atomic<bool> m_isRunning;
    std::atomic<size_t> m_maxBytesPerSec;

    
    std::vector<std::thread> m_workers;
    std::queue<SOCKET> m_jobQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
};

#endif