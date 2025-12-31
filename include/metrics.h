#ifndef METRICS_H
#define METRICS_H

/**
 * @file metrics.h
 * @brief Header for the Metrics tracking class.
 * * Provides real-time analytics for the proxy, including Requests Per Minute (RPM)
 * and domain frequency tracking using a thread-safe sliding window approach.
 */

#include <string>
#include <vector>
#include <cstdint>
#include <utility>

/**
 * @class Metrics
 * @brief Collects and reports proxy usage statistics.
 * * This class maintains a background thread to manage time-based data slots,
 * allowing for accurate RPM calculations and identifying the most
 * frequently visited domains.
 */
class Metrics
{
public:
    /**
     * @brief Initializes the metrics system.
     * @param window_seconds The timeframe for RPM calculation (default: 60s).
     * @param top_k The number of top domains to track (default: 10).
     */
    Metrics(size_t window_seconds = 60, size_t top_k = 10);

    /**
     * @brief Destructor.
     * Ensures the background thread is stopped and implementation memory is freed.
     */
    ~Metrics();

    /**
     * @brief Starts the background 'slot advancer' thread.
     * This thread rotates the time buffers to ensure the RPM stays current.
     */
    void start();

    /**
     * @brief Stops the background thread.
     * Usually called by the destructor, but can be called manually for a clean shutdown.
     */
    void stop();

    /**
     * @brief Records a single request for a specific domain.
     * @param domain The host string (e.g., "example.com"). If empty, it defaults to "unknown".
     */
    void record_request(const std::string &domain);

    /**
     * @brief Calculates the current traffic load.
     * @return The total number of requests handled in the current time window (RPM).
     */
    uint64_t get_rpm() const;

    /**
     * @brief Retrieves the most frequently requested domains.
     * @param k The number of results to return.
     * @return A vector of pairs containing the domain name and the hit count.
     */
    std::vector<std::pair<std::string, uint64_t>> get_top_k(size_t k) const;

private:
    /**
     * @struct Impl
     * @brief Private Implementation structure.
     * Hides the sliding window logic and mutexes from the public interface.
     */
    struct Impl;
    Impl *pimpl; ///< Pointer to the internal metrics data.
};

#endif 