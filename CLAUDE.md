# CLAUDE.md — llm-cache

## Build & Test

```bash
cmake -B build && cmake --build build
./build/basic_cache
```

## Key Constraint: SINGLE HEADER

`include/llm_cache.hpp` is the entire library. Never refactor into multiple files.

## Implementation Guard

```cpp
#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"
```

## Public API

```cpp
namespace llm {

struct CacheConfig {
    std::string cache_dir          = ".llm_cache";
    size_t      max_entries        = 1000;
    int         ttl_seconds        = 86400;    // 0 = never expire
    double      similarity_threshold = 0.95;  // 0 = exact only
};

struct CacheEntry {
    std::string prompt;
    std::string response;
    std::time_t created_at;
    size_t      hit_count;
};

class Cache {
public:
    explicit Cache(CacheConfig config = {});
    std::optional<std::string> get(const std::string& prompt);
    void put(const std::string& prompt, const std::string& response);
    void invalidate(const std::string& prompt);
    void clear();
    struct Stats { size_t hits, misses, total_entries; double hit_rate; size_t bytes_saved; };
    Stats stats() const;
};

std::string cached_call(Cache&, const std::string& prompt,
                        std::function<std::string(const std::string&)> fn);

} // namespace llm
```

## Internals

- FNV-1a hash for prompt -> filename
- Cache files: `{cache_dir}/{hash}.json` (hand-rolled JSON)
- LRU: std::map<time_point, hash> for eviction ordering
- Fuzzy: character 3-gram shingling + Jaccard similarity
- std::mutex for thread safety
