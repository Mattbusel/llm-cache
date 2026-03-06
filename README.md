# llm-cache

Cache LLM responses in C++. Drop in one header. Skip redundant API calls.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![License: MIT](https://img.shields.io/badge/License-MIT-green)
![Single Header](https://img.shields.io/badge/library-single--header-orange)
![Zero deps](https://img.shields.io/badge/deps-none-lightgrey)

---

## 30-second quickstart

```cpp
#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"

#include <iostream>

int main() {
    llm::Cache cache;  // stores to .llm_cache/

    auto my_llm = [](const std::string& prompt) -> std::string {
        // your real API call here (e.g. llm::stream_openai)
        return "Response to: " + prompt;
    };

    // First call: hits the API
    auto r1 = llm::cached_call(cache, "What is recursion?", my_llm);

    // Second call: instant cache hit, zero API cost
    auto r2 = llm::cached_call(cache, "What is recursion?", my_llm);

    auto s = cache.stats();
    std::cout << s.hits << " hits, " << s.misses << " misses\n";
}
```

---

## Installation

Copy one file into your project:

```bash
cp include/llm_cache.hpp your-project/
```

No package manager. No build system changes. No runtime.

---

## API Reference

### Configuration

```cpp
llm::CacheConfig cfg;
cfg.cache_dir            = ".llm_cache"; // disk storage directory
cfg.max_entries          = 1000;         // LRU eviction after this many entries
cfg.ttl_seconds          = 86400;        // expiry (0 = never)
cfg.similarity_threshold = 0.95;         // fuzzy match threshold (0 = exact only)
```

### Cache class

```cpp
llm::Cache cache(cfg);

// Check cache, return hit or nullopt
std::optional<std::string> result = cache.get(prompt);

// Store a response
cache.put(prompt, response);

// Remove one entry
cache.invalidate(prompt);

// Wipe everything
cache.clear();

// Usage statistics
llm::Cache::Stats s = cache.stats();
// s.hits, s.misses, s.hit_rate, s.bytes_saved, s.total_entries
```

### Convenience wrapper

```cpp
// Checks cache first; calls fn(prompt) on miss and stores the result.
std::string response = llm::cached_call(cache, prompt, fn);
```

### Fuzzy matching

When `similarity_threshold > 0`, prompts that differ only slightly (typos,
missing punctuation, rephrasing) will return the cached response for the
most similar stored prompt. Uses character 3-gram Jaccard similarity — no ML needed.

```cpp
cache.put("What is recursion?", "Recursion is...");

// Returns the cached entry despite the slightly different phrasing:
auto r = cache.get("What is recursion");  // similarity ~0.97 -> hit
```

---

## Implementation details

| Feature | Implementation |
|---------|---------------|
| Hash | FNV-1a 64-bit (inline, no dep) |
| Storage | `{cache_dir}/{hash}.json` — hand-rolled JSON, std::fstream |
| Eviction | LRU via `std::map<time_point, hash>` |
| Fuzzy match | Character 3-gram shingling + Jaccard similarity |
| Thread safety | `std::mutex` around all cache operations |
| Dependencies | None (std::filesystem, C++17) |

---

## Examples

| File | Description |
|------|-------------|
| [`examples/basic_cache.cpp`](examples/basic_cache.cpp) | put/get, fuzzy match, stats |
| [`examples/cached_openai.cpp`](examples/cached_openai.cpp) | wraps llm-stream OpenAI calls with cache |

### Building

```bash
cmake -B build && cmake --build build
./build/basic_cache
```

---

## Why

- **Save money.** Repeated or near-identical prompts are free after the first call.
- **Faster iteration.** Dev loops that hit the same prompts don't wait for the network.
- **No runtime.** Pure C++17. Works in any existing C++ build system.

---

## See Also

This library is part of the **llm-cpp suite** — a set of drop-in single-header C++ libraries for working with LLM APIs:

| Repo | What it does |
|------|-------------|
| [llm-stream](https://github.com/Mattbusel/llm-stream) | Stream OpenAI & Anthropic responses token by token |
| **llm-cache** | Cache responses, skip redundant calls, save money |
| [llm-cost](https://github.com/Mattbusel/llm-cost) | Token counting + cost estimation before you send |
| [llm-retry](https://github.com/Mattbusel/llm-retry) | Retry with backoff + circuit breaker |
| [llm-format](https://github.com/Mattbusel/llm-format) | Structured output (JSON schema enforcement) |

---

## Requirements

- C++17 or later
- No external dependencies

---

## License

MIT — see [LICENSE](LICENSE).
