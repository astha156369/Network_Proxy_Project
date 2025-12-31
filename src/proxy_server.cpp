// src/proxy_server.cpp
#include "proxy_server.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

#include "filter_manager.h"
#include "logger.h"
#include "metrics.h"

#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

static inline std::string trim(const std::string &s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace((unsigned char)s[start]))
        ++start;
    size_t end = s.size();
    while (end > start && std::isspace((unsigned char)s[end - 1]))
        --end;
    return s.substr(start, end - start);
}

static inline std::string to_lower(const std::string &s)
{
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c)
                   { return (char)std::tolower(c); });
    return r;
}

static bool send_all(SOCKET sock, const char *data, size_t length)
{
    size_t sent = 0;
    while (sent < length)
    {
        int ret = send(sock, data + sent, (int)(length - sent), 0);
        if (ret <= 0)
            return false;
        sent += (size_t)ret;
    }
    return true;
}

/** Graceful close with linger to flush data before closing */
static void graceful_close(SOCKET s)
{
    if (s == INVALID_SOCKET)
        return;
    linger solinger;
    solinger.l_onoff = 1;
    solinger.l_linger = 1;
    setsockopt(s, SOL_SOCKET, SO_LINGER, (const char *)&solinger, sizeof(solinger));

    shutdown(s, SD_SEND);

    char drain[1024];
    while (recv(s, drain, sizeof(drain), 0) > 0)
        ;

    closesocket(s);
}

static void forward_loop(SOCKET src, SOCKET dst, size_t limit)
{
    char buf[8192];
    auto start_time = std::chrono::steady_clock::now();
    size_t total_sent = 0;
    while (true)
    {
        int r = recv(src, buf, sizeof(buf), 0);
        if (r <= 0)
            break;
        if (!send_all(dst, buf, (size_t)r))
            break;

        if (limit > 0)
        {
            total_sent += (size_t)r;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
            double expected = (total_sent / (double)limit) * 1000.0;
            if (elapsed < expected)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds((int)(expected - elapsed)));
            }
            if (elapsed > 5000)
            {
                start_time = std::chrono::steady_clock::now();
                total_sent = 0;
            }
        }
    }
    shutdown(dst, SD_SEND);
}

static void run_tunnel(SOCKET client, SOCKET server, size_t limit)
{
    std::thread t1(forward_loop, client, server, limit);
    std::thread t2(forward_loop, server, client, limit);
    if (t1.joinable())
        t1.join();
    if (t2.joinable())
        t2.join();
    graceful_close(server);
    graceful_close(client);
}

static FilterManager filterManager;
static Logger logger;
static Metrics metrics;

// Helper: unified per-request logging (file + concise console output)
static void log_request(const std::string &client_desc,
                        const std::string &dest,
                        const std::string &reqline,
                        const std::string &action,
                        int status,
                        size_t bytes)
{
    // write to persistent logger
    logger.log(client_desc, dest, reqline, action, status, bytes);

    // concise console line for per-request visibility
    std::ostringstream oss;
    oss << "[REQ] " << client_desc << " -> " << dest << " \"" << reqline << "\" " << action << " " << status << " bytes=" << bytes;
    std::cout << oss.str() << std::endl;
    std::cout.flush();
}

ProxyServer::ProxyServer(int port)
    : m_port(port), m_listenSocket(INVALID_SOCKET), m_isRunning(false), m_maxBytesPerSec(0) {}

ProxyServer::~ProxyServer() { stop(); }

void ProxyServer::worker_thread()
{
    while (m_isRunning)
    {
        SOCKET client;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait(lock, [this]
                             { return !m_jobQueue.empty() || !m_isRunning; });
            if (!m_isRunning)
                return;
            client = m_jobQueue.front();
            m_jobQueue.pop();
        }
        handle_client(client);
    }
}

void ProxyServer::stop()
{
    m_isRunning = false;
    m_condition.notify_all();
    for (auto &t : m_workers)
        if (t.joinable())
            t.join();
    if (m_listenSocket != INVALID_SOCKET)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
    WSACleanup();
}

void ProxyServer::start()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(m_port);

    bind(m_listenSocket, (sockaddr *)&addr, sizeof(addr));
    listen(m_listenSocket, SOMAXCONN);

    filterManager.load("config/blocked_domains.txt");
    logger.init("logs/proxy.log");
    metrics.start();

    m_isRunning = true;
    for (int i = 0; i < 20; ++i)
        m_workers.emplace_back(&ProxyServer::worker_thread, this);

    // admin thread for /metrics (unchanged)
    std::thread([this]()
                {
        SOCKET admin = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(8889); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        bind(admin, (sockaddr*)&a, sizeof(a));
        listen(admin, 5);

        char buf[4096];
        while (m_isRunning) {
            SOCKET c = accept(admin, nullptr, nullptr);
            if (c == INVALID_SOCKET) break;

            int n = recv(c, buf, sizeof(buf)-1, 0);
            if (n > 0) {
                buf[n] = '\0';
                std::string req(buf);
                std::string body;
                std::string contentType = "text/plain";

                if (req.find("GET /metrics") != std::string::npos) {
                    auto top = metrics.get_top_k(5);
                    std::ostringstream oss;
                    oss << "{\"rpm\":" << metrics.get_rpm() << ",\"limit\":" << m_maxBytesPerSec.load() << ",\"top\":[";
                    for(size_t i=0; i<top.size(); ++i) 
                        oss << "[\"" << top[i].first << "\"," << top[i].second << "]" << (i==top.size()-1?"":",");
                    oss << "]}";
                    body = oss.str();
                    contentType = "application/json";
                } else if (req.find("speed=") != std::string::npos) {
                    size_t pos = req.find("speed=");
                    std::string v = req.substr(pos + 6);
                    size_t space = v.find_first_not_of("0123456789");
                    if (space != std::string::npos) v = v.substr(0, space);
                    
                    if(!v.empty()) { 
                        m_maxBytesPerSec.store(std::stoul(v)); 
                        body = "SUCCESS: Speed updated to " + v + " B/s\r\n"; 
                    }
                }

                if(!body.empty()){
                    std::ostringstream res;
                    res << "HTTP/1.1 200 OK\r\n"
                        << "Content-Type: " << contentType << "\r\n"
                        << "Content-Length: " << body.size() << "\r\n"
                        << "Connection: close\r\n\r\n"
                        << body;
                    std::string sRes = res.str();
                    send_all(c, sRes.c_str(), sRes.size());
                }
            }
            graceful_close(c); 
        }
        closesocket(admin); })
        .detach();

    // Accept loop: push accepted sockets to worker queue (no per-connection console prints)
    while (m_isRunning)
    {
        sockaddr_storage clientAddrStorage{};
        socklen_t clientSize = static_cast<socklen_t>(sizeof(clientAddrStorage));
        SOCKET client = accept(m_listenSocket, reinterpret_cast<sockaddr *>(&clientAddrStorage), &clientSize);
        if (client == INVALID_SOCKET)
        {
            // accept failed or interrupted; continue loop
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_jobQueue.push(client);
        }
        m_condition.notify_one();
    }
}

void ProxyServer::handle_client(SOCKET clientSocket)
{
    // Build client description string once per connection (used for all request logs)
    std::string client_desc = "unknown";
    {
        sockaddr_storage peer{};
        socklen_t plen = sizeof(peer);
        if (getpeername(clientSocket, reinterpret_cast<sockaddr *>(&peer), &plen) == 0)
        {
            char ipbuf[INET6_ADDRSTRLEN] = {0};
            uint16_t port = 0;
            if (peer.ss_family == AF_INET)
            {
                sockaddr_in *sa = reinterpret_cast<sockaddr_in *>(&peer);
                inet_ntop(AF_INET, &sa->sin_addr, ipbuf, sizeof(ipbuf));
                port = ntohs(sa->sin_port);
            }
            else if (peer.ss_family == AF_INET6)
            {
                sockaddr_in6 *sa6 = reinterpret_cast<sockaddr_in6 *>(&peer);
                inet_ntop(AF_INET6, &sa6->sin6_addr, ipbuf, sizeof(ipbuf));
                port = ntohs(sa6->sin6_port);
            }
            if (ipbuf[0] != 0)
            {
                std::ostringstream cd;
                cd << ipbuf << ":" << port;
                client_desc = cd.str();
            }
        }
    }

    DWORD timeout = 10000;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    char buffer[8192];
    std::string requestData;
    while (requestData.find("\r\n\r\n") == std::string::npos)
    {
        int br = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (br <= 0)
        {
            closesocket(clientSocket);
            return;
        }
        requestData.append(buffer, br);
        if (requestData.size() > 65536)
        {
            closesocket(clientSocket);
            return;
        }
    }

    std::istringstream rs(requestData);
    std::string reqLine;
    std::getline(rs, reqLine);
    if (!reqLine.empty() && reqLine.back() == '\r')
        reqLine.pop_back();
    std::istringstream rl(reqLine);
    std::string method, target, version;
    rl >> method >> target >> version;

    std::map<std::string, std::string> headers;
    std::string line;
    while (std::getline(rs, line) && line != "\r" && !line.empty())
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        size_t pos = line.find(':');
        if (pos != std::string::npos)
            headers[to_lower(trim(line.substr(0, pos)))] = trim(line.substr(pos + 1));
    }

    std::string host, port = (method == "CONNECT" ? "443" : "80");
    if (method == "CONNECT")
    {
        host = (target.find(':') != std::string::npos) ? target.substr(0, target.find(':')) : target;
        if (target.find(':') != std::string::npos)
            port = target.substr(target.find(':') + 1);
    }
    else
    {
        auto it = headers.find("host");
        if (it != headers.end())
        {
            host = (it->second.find(':') != std::string::npos) ? it->second.substr(0, it->second.find(':')) : it->second;
            if (it->second.find(':') != std::string::npos)
                port = it->second.substr(it->second.find(':') + 1);
        }
    }

    // If host is empty treat as malformed request
    if (host.empty())
    {
        logger.log(client_desc, "", reqLine, "ERROR", 400, 0);
        graceful_close(clientSocket);
        return;
    }

    metrics.record_request(host);

    // Apply filter per request
    if (filterManager.is_blocked(host))
    {
        std::string res = "HTTP/1.1 403 Forbidden\r\nContent-Length: 9\r\nConnection: close\r\n\r\nForbidden";
        send_all(clientSocket, res.data(), res.size());
        log_request(client_desc, host + ":" + port, reqLine, "BLOCKED", 403, 0);
        graceful_close(clientSocket);
        return;
    }

    addrinfo hints{}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0)
    {
        // DNS resolution failed -> record request-level error
        log_request(client_desc, host + ":" + port, reqLine, "ERROR", 502, 0);
        graceful_close(clientSocket);
        return;
    }

    SOCKET serverSock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    setsockopt(serverSock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    if (connect(serverSock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR)
    {
        freeaddrinfo(res);
        log_request(client_desc, host + ":" + port, reqLine, "ERROR", 502, 0);
        graceful_close(serverSock);
        graceful_close(clientSocket);
        return;
    }
    freeaddrinfo(res);

    size_t limit = m_maxBytesPerSec.load();
    if (method == "CONNECT")
    {
        // Log the CONNECT request as a request-level FORWARD (200)
        log_request(client_desc, host + ":" + port, reqLine, "FORWARD", 200, 0);

        std::string ok = "HTTP/1.1 200 Connection Established\r\n\r\n";
        send_all(clientSocket, ok.data(), ok.size());
        run_tunnel(clientSocket, serverSock, limit);
    }
    else
    {
        std::ostringstream reqOut;
        // If client sent absolute URI as target, preserve path portion; if not, use target as-is.
        reqOut << method << " " << target << " " << version << "\r\n";
        for (auto const &kv : headers)
        {
            const std::string &k = kv.first;
            const std::string &v = kv.second;
            if (k != "connection" && k != "proxy-connection")
                reqOut << k << ": " << v << "\r\n";
        }
        reqOut << "Connection: close\r\n\r\n";
        std::string finalReq = reqOut.str();
        send_all(serverSock, finalReq.data(), finalReq.size());

        size_t total = 0;
        auto r_start = std::chrono::steady_clock::now();
        while (true)
        {
            int r = recv(serverSock, buffer, sizeof(buffer), 0);
            if (r <= 0)
                break;
            if (!send_all(clientSocket, buffer, (size_t)r))
                break;
            total += (size_t)r;
            if (limit > 0)
            {
                auto now = std::chrono::steady_clock::now();
                auto elap = std::chrono::duration_cast<std::chrono::milliseconds>(now - r_start).count();
                double exp = (total / (double)limit) * 1000.0;
                if (elap < exp)
                    std::this_thread::sleep_for(std::chrono::milliseconds((int)(exp - elap)));
            }
        }
        // Request-level final log
        log_request(client_desc, host + ":" + port, reqLine, "FORWARD", 200, total);
        graceful_close(serverSock);
        graceful_close(clientSocket);
    }
}
