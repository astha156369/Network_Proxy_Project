#include "metrics.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <queue>
#include <sstream>

using namespace std::chrono;

struct Metrics::Impl {
    size_t window_seconds;
    size_t slots_count;
    std::vector<std::atomic<uint64_t>> slots; 
    std::atomic<size_t> current_slot;
    std::atomic<bool> running;
    std::thread adv_thread;

    mutable std::mutex domain_mtx;
    std::unordered_map<std::string, uint64_t> domain_counts;

    size_t top_k_default;

    Impl(size_t ws, size_t topk)
        : window_seconds(ws),
          slots_count(ws),
          slots(ws),
          current_slot(0),
          running(false),
          top_k_default(topk)
    {
        for (size_t i = 0; i < slots_count; ++i) slots[i] = 0;
    }
};

Metrics::Metrics(size_t window_seconds, size_t top_k)
    : pimpl(new Impl(window_seconds, top_k))
{}

Metrics::~Metrics() {
    stop();
    delete pimpl;
}

void Metrics::start() {
    bool expected = false;
    if (!pimpl->running.compare_exchange_strong(expected, true)) return;

    pimpl->adv_thread = std::thread([this]() {
        while (pimpl->running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            size_t next = (pimpl->current_slot.load() + 1) % pimpl->slots_count;

            pimpl->slots[next].store(0, std::memory_order_relaxed);
            pimpl->current_slot.store(next, std::memory_order_relaxed);
        }
    });
}

void Metrics::stop() {
    bool expected = true;
    if (pimpl->running.compare_exchange_strong(expected, false)) {
        if (pimpl->adv_thread.joinable()) pimpl->adv_thread.join();
    }
}

void Metrics::record_request(const std::string &domain) {

    size_t idx = pimpl->current_slot.load(std::memory_order_relaxed);
    pimpl->slots[idx].fetch_add(1, std::memory_order_relaxed);

   
    std::string d = domain.empty() ? "unknown" : domain;
   
    for (auto &c : d) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    {
        std::lock_guard<std::mutex> lg(pimpl->domain_mtx);
        pimpl->domain_counts[d] += 1;
    }
}

uint64_t Metrics::get_rpm() const {
    uint64_t sum = 0;
    size_t cur = pimpl->current_slot.load(std::memory_order_relaxed);
    for (size_t i = 0; i < pimpl->slots_count; ++i) {
        sum += pimpl->slots[i].load(std::memory_order_relaxed);
    }
    return sum;
}

std::vector<std::pair<std::string, uint64_t>> Metrics::get_top_k(size_t k) const {
    std::vector<std::pair<std::string, uint64_t>> out;
    {
        std::lock_guard<std::mutex> lg(pimpl->domain_mtx);
        out.reserve(pimpl->domain_counts.size());
        for (auto &p : pimpl->domain_counts) out.emplace_back(p.first, p.second);
    }
    if (out.empty()) return {};
    
    std::sort(out.begin(), out.end(), [](const auto &a, const auto &b){
        return a.second > b.second;
    });
    if (out.size() > k) out.resize(k);
    return out;
}
