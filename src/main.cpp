/**
 * @file main.cpp
 * @author Your Name
 * @brief Entry point for the Custom C++ Proxy Server.
 * @version 1.0
 * @date 2025-12-22
 */

#include "proxy_server.h"
#include <iostream>

/**
 * @brief Main entry point of the application.
 * * Initializes the ProxyServer class and starts the listening loop.
 */
int main()
{
    
    ProxyServer server(8888);

 
    std::cout << "=======================================" << std::endl;
    std::cout << "   CUSTOM NETWORK PROXY SERVER v1.0    " << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << "[INFO] System Ready." << std::endl;
    std::cout << "[INFO] Listening on port 8888..." << std::endl;
    std::cout << "[HINT] Press Ctrl+C to shut down the server." << std::endl;
    std::cout << "---------------------------------------" << std::endl;


    server.start();

    return 0;
}