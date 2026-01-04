# Custom Network Proxy Server

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)
[![License](https://img.shields.io/badge/License-See%20LICENSE%20file-green.svg)](LICENSE)

A high-performance forward proxy server built from scratch using C++ and the Windows Sockets (Winsock) API. This proxy server handles standard HTTP requests and secure HTTPS tunneling via the `CONNECT` method, making it suitable for network monitoring, content filtering, and traffic analysis.

## Table of Contents

- [What the Project Does](#what-the-project-does)
- [Why the Project is Useful](#why-the-project-is-useful)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Building](#building)
  - [Running](#running)
  - [Configuration](#configuration)
- [Usage Examples](#usage-examples)
- [Features](#features)
- [Project Structure](#project-structure)
- [Getting Help](#getting-help)
- [Contributing](#contributing)

## What the Project Does

This project implements a **forward proxy server** that acts as an intermediary between clients and destination servers. When a client makes an HTTP or HTTPS request through the proxy, the proxy:

- Intercepts the client's request
- Forwards it to the target server
- Returns the server's response back to the client
- Logs all activity for monitoring and analysis

The proxy supports:
- **HTTP forwarding**: Standard HTTP GET, POST, and other methods
- **HTTPS tunneling**: Secure TCP tunneling using the HTTP `CONNECT` method
- **Domain filtering**: Block or allow requests based on domain/IP rules
- **Real-time metrics**: Monitor traffic patterns and performance
- **Bandwidth throttling**: Control data transfer rates

## Why the Project is Useful

### Key Benefits

1. **Network Control**: Filter and block unwanted domains or IP addresses for security and compliance
2. **Traffic Monitoring**: Real-time metrics help understand network usage patterns
3. **Performance Analysis**: Track requests per minute (RPM) and identify top-visited domains
4. **Educational Value**: Clean, well-structured C++ code demonstrating socket programming, multithreading, and proxy architecture
5. **Lightweight**: Native C++ implementation with minimal dependencies, suitable for resource-constrained environments
6. **Extensible**: Modular design makes it easy to add new features like authentication, caching, or custom filtering logic

### Use Cases

- Corporate network filtering and monitoring
- Development and testing environments
- Learning network programming and proxy architecture
- Traffic analysis and debugging
- Bandwidth management and throttling

## Getting Started

### Prerequisites

- **Compiler**: MinGW-w64 (g++) with C++17 support
- **Operating System**: Windows 10/11
- **Tools**: 
  - PowerShell (for running test scripts)
  - `curl` (for manual testing, optional but recommended)

### Building

#### Option 1: Using Makefile

```powershell
make
```

#### Option 2: Manual Compilation

```powershell
g++ -std=c++17 -O2 -Wall -Iinclude src\main.cpp src\proxy_server.cpp src\filter_manager.cpp src\logger.cpp src\metrics.cpp src\thread_pool.cpp -lws2_32 -o proxy.exe
```

The build process compiles all source files and links against the Windows Sockets library (`ws2_32`). The output executable will be `proxy.exe` in the project root.

### Running

1. **Start the proxy server**:

```powershell
.\proxy.exe
```

The server will start listening on port `8888` by default. You should see:

```
=======================================
   CUSTOM NETWORK PROXY SERVER v1.0    
=======================================
[INFO] System Ready.
[INFO] Listening on port 8888...
[HINT] Press Ctrl+C to shut down the server.
---------------------------------------
```

2. **Configure your client** to use the proxy at `localhost:8888`

3. **Access metrics** (optional): The admin server runs on port `8889` for real-time metrics

### Configuration

#### Blocked Domains

Edit `config/blocked_domains.txt` to add domains or IPs you want to block. The file supports:

- **Exact domains**: `example.com`
- **Wildcard domains**: `*.example.com` (blocks all subdomains)
- **IP addresses**: `192.0.2.5`

Example configuration:

```
# Blocked Domains Configuration
blocked.com
badsite.net
*.bad-analytics.org
192.0.2.5
```

The proxy will return a `403 Forbidden` response for blocked domains.

## Usage Examples

### Basic HTTP Request (Valid Connection)

```powershell
curl.exe -x 127.0.0.1:8888 http://httpbin.org/get
```

This demonstrates a successful HTTP request through the proxy. The proxy forwards the request to httpbin.org and returns the response.

### Blocked Site (Forbidden Response)

```powershell
curl.exe -x localhost:8888 http://example.com
```

If `example.com` is in the blocked domains list, the proxy will return a `403 Forbidden` response without forwarding the request.

### Invalid Domain (Error Handling)

```powershell
curl.exe -v -x 127.0.0.1:8888 http://nonexistent123456789.com
```

This demonstrates error handling when DNS resolution fails. The `-v` flag shows verbose connection details including the error.

### HTTPS Request (via CONNECT Tunneling)

```powershell
curl -x http://127.0.0.1:8888 https://www.google.com
```

This demonstrates HTTPS tunneling using the HTTP CONNECT method. The proxy establishes a TCP tunnel between the client and the target server for encrypted traffic.

### Check Proxy Metrics

```powershell
curl.exe http://localhost:8889/metrics
```

Response example:
```json
{
  "rpm": 42,
  "limit": 0,
  "top": [
    ["example.com", 15],
    ["httpbin.org", 10],
    ["google.com", 8]
  ]
}
```

### Set Bandwidth Limit

```powershell
curl.exe "http://localhost:8889/speed=1024000"
```

This sets the bandwidth limit to 1 MB/s (1024000 bytes per second).

### Testing with Test Scripts

Run the automated test suite:

```powershell
.\build-and-test.ps1
```

Or run individual test scripts:

```powershell
# Basic functionality tests
.\tests\basic_tests.ps1

# CONNECT method and filtering tests
.\tests\connect_tests.ps1

# Concurrency and stress tests
.\tests\concurrency_test.ps1
```

## Features

### Multithreaded Architecture

- **Thread pool**: Efficiently handles multiple concurrent connections
- **Thread-safe operations**: All shared resources are protected with mutexes
- **Graceful shutdown**: Clean termination of worker threads

### HTTP/HTTPS Support

- **HTTP forwarding**: Full support for standard HTTP methods (GET, POST, PUT, DELETE, etc.)
- **HTTPS tunneling**: Implements the HTTP `CONNECT` method for secure TCP tunneling
- **Request/Response parsing**: Properly handles HTTP headers and status codes

### Domain Filtering

- **Flexible rules**: Support for exact matches, wildcards, and IP addresses
- **Runtime configuration**: Reload blocking rules without restarting
- **Thread-safe filtering**: Concurrent request checking without race conditions

### Real-time Metrics

- **Requests Per Minute (RPM)**: Track traffic load in real-time
- **Top domains**: Identify most frequently accessed destinations
- **Sliding window**: Accurate time-based calculations
- **Admin API**: JSON endpoint for programmatic access

### Logging

- **Thread-safe logging**: All operations logged without conflicts
- **Structured format**: Easy-to-parse log entries
- **Request tracking**: Logs include client info, target host, request line, action, status, and bytes transferred
- **Log file**: All activity written to `logs/proxy.log`

### Bandwidth Throttling

- **Configurable limits**: Set maximum bytes per second
- **Dynamic adjustment**: Change limits via admin API without restart
- **Per-connection throttling**: Fair distribution across connections

## Project Structure

```
proxy-project/
├── include/              # Header files
│   ├── proxy_server.h   # Main proxy server class
│   ├── filter_manager.h # Domain filtering logic
│   ├── logger.h         # Logging functionality
│   ├── metrics.h        # Metrics tracking
│   └── thread_pool.h    # Thread pool implementation
├── src/                 # Source files
│   ├── main.cpp         # Entry point
│   ├── proxy_server.cpp # Core proxy logic
│   ├── filter_manager.cpp
│   ├── logger.cpp
│   ├── metrics.cpp
│   └── thread_pool.cpp
├── config/              # Configuration files
│   ├── server.conf      # Server configuration
│   └── blocked_domains.txt  # Domain blacklist
├── tests/               # Test scripts
│   ├── basic_tests.ps1
│   ├── connect_tests.ps1
│   └── concurrency_test.ps1
├── docs/                # Documentation
│   ├── design.md        # Architecture documentation
│   └── demo.md          # Usage demonstrations
├── logs/                # Log files (generated at runtime)
├── Makefile             # Build configuration
└── README.md            # This file
```

## Getting Help

### Documentation

- **[Design Document](docs/design.md)**: Detailed architecture and component overview
- **[Demo Guide](docs/demo.md)**: Visual examples and verification steps
- **[Screenshots Documentation](docs/screenshots.md)**: Visual documentation of proxy server scenarios and use cases

### Troubleshooting

**Proxy won't start:**
- Ensure port 8888 is not already in use
- Check that Winsock is properly initialized
- Verify the executable was built successfully

**Requests are blocked unexpectedly:**
- Review `config/blocked_domains.txt` for matching rules
- Check wildcard patterns (e.g., `*.example.com` matches `sub.example.com`)
- Verify the configuration file is in the correct location

**High memory usage:**
- Monitor the number of concurrent connections
- Consider adjusting thread pool size
- Check for connection leaks in logs

**Metrics not updating:**
- Ensure the admin server is running (port 8889)
- Check that requests are being processed (verify in logs)
- Restart the proxy if metrics appear stuck

### Logs

All proxy activity is logged to `logs/proxy.log`. Check this file for:
- Request details
- Error messages
- Connection status
- Filtering decisions

### Reporting Issues

If you encounter bugs or have feature requests:
1. Check existing documentation and logs
2. Review the design document for architecture details
3. Create an issue with:
   - Steps to reproduce
   - Expected vs. actual behavior
   - Relevant log excerpts
   - System information (Windows version, compiler version)

## Contributing

Contributions are welcome! This project benefits from community input.

### How to Contribute

1. **Fork the repository** and create a feature branch
2. **Follow the existing code style**:
   - Use C++17 features
   - Maintain thread safety
   - Add comments for complex logic
   - Keep functions focused and modular
3. **Test your changes**:
   - Run the test suite: `.\build-and-test.ps1`
   - Verify new features work with both HTTP and HTTPS
   - Check that logging and metrics still function correctly
4. **Update documentation** if you add features or change behavior
5. **Submit a pull request** with a clear description of changes

### Areas for Contribution

- Performance optimizations
- Additional filtering rules (regex support, time-based rules)
- Authentication mechanisms
- Caching functionality
- Cross-platform support (Linux/macOS)
- Enhanced metrics and monitoring
- Unit tests
- Code documentation improvements

### Code Style Guidelines

- Use meaningful variable and function names
- Prefer `const` where possible
- Use RAII for resource management
- Document public APIs with comments
- Keep functions under 100 lines when possible
- Use the Pimpl pattern for complex internal implementations

---

**Note**: This proxy server is designed for educational and development purposes. For production use, consider additional security features such as authentication, encryption, and comprehensive error handling.
