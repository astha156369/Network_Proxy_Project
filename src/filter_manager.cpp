#include "filter_manager.h"
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <memory>

struct FilterManager::Impl
{
    std::vector<std::string> exact;  
    std::vector<std::string> suffix; 
    std::mutex m;
};

static inline std::string trim(const std::string &s)
{
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a])))
        ++a;
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
        --b;
    return s.substr(a, b - a);
}

static inline std::string lower(const std::string &s)
{
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c)
                   { return std::tolower(c); });
    return r;
}

FilterManager::FilterManager() : pimpl(new Impl()) {}
FilterManager::~FilterManager() { delete pimpl; }

bool FilterManager::load(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open())
        return false;
    std::vector<std::string> exact_local;
    std::vector<std::string> suffix_local;
    std::string line;
    while (std::getline(ifs, line))
    {
        line = trim(line);
        if (line.empty())
            continue;
        if (line[0] == '#')
            continue;
        std::string s = lower(line);
        if (s.rfind("*.", 0) == 0 && s.size() > 2)
        {
           
            suffix_local.push_back(s.substr(2));
        }
        else
        {
            exact_local.push_back(s);
        }
    }
    
    {
        std::lock_guard<std::mutex> lg(pimpl->m);
        pimpl->exact.swap(exact_local);
        pimpl->suffix.swap(suffix_local);
    }
    return true;
}

bool FilterManager::is_blocked(const std::string &hostOrIp) const
{
    if (hostOrIp.empty())
        return false;
    std::string h = lower(trim(hostOrIp));
    std::lock_guard<std::mutex> lg(pimpl->m);
   
    for (const auto &e : pimpl->exact)
    {
        if (e == h)
            return true;
    }
    
    for (const auto &suf : pimpl->suffix)
    {
        if (h.size() >= suf.size() + 1)
        {
            
            if (h.size() == suf.size())
                continue; 
            if (h.compare(h.size() - suf.size(), suf.size(), suf) == 0)
            {
                
                size_t pos = h.size() - suf.size();
                if (pos == 0)
                    return true;
                if (h[pos - 1] == '.')
                    return true;
            }
        }
        else if (h == suf)
        {
            return true;
        }
    }
    return false;
}
