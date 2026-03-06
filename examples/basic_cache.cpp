#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"

#include <iostream>
#include <thread>

int main() {
    llm::CacheConfig cfg;
    cfg.cache_dir            = ".llm_cache_demo";
    cfg.max_entries          = 100;
    cfg.ttl_seconds          = 3600;
    cfg.similarity_threshold = 0.85; // fuzzy match within 85% similarity

    llm::Cache cache(cfg);

    // Simulate an LLM call (just returns a canned response)
    auto fake_llm = [](const std::string& prompt) -> std::string {
        std::cout << "  [API call made for: \"" << prompt << "\"]\n";
        return "Response to: " + prompt;
    };

    std::cout << "=== llm-cache basic demo ===\n\n";

    // First call — cache miss, fn is invoked
    std::cout << "Call 1 (expect miss):\n";
    auto r1 = llm::cached_call(cache, "What is the capital of France?", fake_llm);
    std::cout << "Result: " << r1 << "\n\n";

    // Second call — exact cache hit, fn is NOT invoked
    std::cout << "Call 2 (expect hit, no API call):\n";
    auto r2 = llm::cached_call(cache, "What is the capital of France?", fake_llm);
    std::cout << "Result: " << r2 << "\n\n";

    // Fuzzy match — similar prompt, should hit via shingling
    std::cout << "Call 3 (fuzzy match, expect hit):\n";
    auto r3 = llm::cached_call(cache, "What is the capital of France", fake_llm); // no '?'
    std::cout << "Result: " << r3 << "\n\n";

    // Different prompt — cache miss
    std::cout << "Call 4 (different prompt, expect miss):\n";
    auto r4 = llm::cached_call(cache, "What is the capital of Germany?", fake_llm);
    std::cout << "Result: " << r4 << "\n\n";

    // Direct put/get
    cache.put("custom prompt", "custom response");
    auto direct = cache.get("custom prompt");
    std::cout << "Direct get: " << (direct ? *direct : "(miss)") << "\n\n";

    // Invalidate
    cache.invalidate("custom prompt");
    auto after_invalidate = cache.get("custom prompt");
    std::cout << "After invalidate: " << (after_invalidate ? *after_invalidate : "(miss)") << "\n\n";

    // Stats
    auto s = cache.stats();
    std::cout << "=== Stats ===\n"
              << "Hits:          " << s.hits          << "\n"
              << "Misses:        " << s.misses        << "\n"
              << "Total entries: " << s.total_entries << "\n"
              << "Hit rate:      " << static_cast<int>(s.hit_rate * 100) << "%\n"
              << "Bytes saved:   " << s.bytes_saved   << "\n";

    return 0;
}
