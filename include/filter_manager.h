#ifndef FILTER_MANAGER_H
#define FILTER_MANAGER_H

/**
 * @file filter_manager.h
 * @brief Header for the FilterManager class.
 * * Provides domain and IP-based filtering logic to intercept and block
 * restricted traffic based on a configuration file.
 */

#include <string>

/**
 * @class FilterManager
 * @brief Handles the security logic for domain-based filtering.
 * * This class reads a blacklist from a file and provides a thread-safe
 * method to check if a specific host or IP should be intercepted.
 */
class FilterManager
{
public:
    /**
     * @brief Default constructor.
     */
    FilterManager();

    /**
     * @brief Destructor.
     * Handles the clean release of the internal implementation pointer.
     */
    ~FilterManager();

    /**
     * @brief Loads blocking rules from a text file.
     * * Parses the file at the given path. Supports:
     * - Exact domains: example.com
     * - Wildcard domains: *.example.com
     * - Exact IPs: 192.0.2.5
     * * @param path The relative or absolute path to blocked_domains.txt.
     * @return true if the file was successfully opened and parsed, false otherwise.
     */
    bool load(const std::string &path);

    /**
     * @brief Evaluates whether a target should be blocked.
     * * Compares the provided host or IP against the loaded ruleset.
     * * @param hostOrIp The string to check (e.g., "badsite.com" or "10.0.0.1").
     * @return true if the target is found in the blacklist (block it), false if allowed.
     */
    bool is_blocked(const std::string &hostOrIp) const;

private:
    /**
     * @struct Impl
     * @brief Private Implementation structure (Pimpl pattern).
     * * This hides the internal data structures (like std::set or std::vector)
     * from the header file to reduce compilation dependencies.
     */
    struct Impl;
    Impl *pimpl = nullptr; ///< Pointer to the actual data storage.
};

#endif // FILTER_MANAGER_H