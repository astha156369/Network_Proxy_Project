# Custom Network Proxy Server (C++ / Winsock)

##  Overview

This is a high-performance forward proxy server built from scratch using C++ and the Windows Sockets (Winsock) API. It supports standard HTTP requests and secure HTTPS tunneling via the `CONNECT` method.

##  Key Features

- **Multithreaded Architecture**: Handles multiple concurrent client connections using a thread-per-connection model.
- **HTTPS Support**: Implements TCP tunneling for secure traffic.
- **Domain Filtering**: Rule-based blocking via `config/blocked_domains.txt`.
- **Live Metrics**: A built-in admin server on port `8889` reporting Requests Per Minute (RPM) and top-visited domains.
- **Robust Logging**: Thread-safe logging to `logs/proxy.log`.

## 🛠 Prerequisites

- **Compiler**: MinGW-w64 (g++)
- **OS**: Windows 10/11
- **Tools**: PowerShell (for running test scripts) and `curl` (for manual testing).

##  Building and Running

### 1. Build the Proxy

Open PowerShell in the project root and run:

```powershell
g++ -Iinclude src\main.cpp src\proxy_server.cpp src\filter_manager.cpp src\logger.cpp src\metrics.cpp -lws2_32 -o proxy.exe
```
