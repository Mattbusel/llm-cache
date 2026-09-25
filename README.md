# llm-cache

[![CI](https://github.com/Mattbusel/llm-cache/actions/workflows/ci.yml/badge.svg)](https://github.com/Mattbusel/llm-cache/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Single header](https://img.shields.io/badge/single-header-green.svg)

A thread-safe LRU cache with TTL for LLM responses, so repeated prompts stop costing money.

> Part of **[llm-cpp](https://github.com/Mattbusel/llm-cpp)**, a family of 26 single-header C++ libraries for building on LLM APIs. Each one stands alone: copy one header, include it, done.

Many LLM apps send the same prompt more than once: retries, repeated user questions, test runs. llm-cache keeps recent responses in memory with an LRU limit and expiry time, and `get_or_compute()` only calls your API function on a miss. It has no network code and no dependencies, so it wraps whatever client you already use.

## Features

- LRU eviction with a configurable `max_entries` (default 1000)
- Time-to-live per entry (`ttl_seconds`, default 3600; 0 disables expiry)
- Optional case-insensitive keys (`case_sensitive = false` by default)
- `get_or_compute(key, fn)`: return the cached value or call `fn`, store and return its result
- Hit/miss/eviction counters and `hit_rate()`
- Mutex-protected, safe to share across threads
- `prompt_hash()`: 64-bit FNV-1a hash of a prompt, hex-encoded, for compact keys

## Quick start

Requirements: a C++17 compiler. No other dependencies, no network access.

1. Copy [`include/llm_cache.hpp`](include/llm_cache.hpp) into your project.
2. In exactly one `.cpp` file, `#define LLM_CACHE_IMPLEMENTATION` before including it. Other files just `#include "llm_cache.hpp"`.

```cpp
#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"
#include <iostream>

std::string call_llm(const std::string& prompt) {
    return "response to: " + prompt;  // your real API call goes here
}

int main() {
    llm::CacheConfig cfg;
    cfg.max_entries = 500;
    cfg.ttl_seconds = 600;  // entries expire after 10 minutes
    llm::ResponseCache cache(cfg);

    std::string prompt = "What is the capital of France?";
    std::string key = llm::prompt_hash(prompt);

    cache.get_or_compute(key, [&] { return call_llm(prompt); });           // miss: calls the API
    std::string answer = cache.get_or_compute(key, [&] { return call_llm(prompt); });  // hit

    std::cout << answer << "\nhit rate: " << cache.stats().hit_rate() << "\n";  // 0.5
}
```

Build and run:

```bash
g++ -std=c++17 -I include example.cpp -o example
./example
```

Output:

```text
response to: What is the capital of France?
hit rate: 0.5
```

## API

Everything lives in namespace `llm`.

| Function / type | What it does |
|---|---|
| `ResponseCache(CacheConfig)` | Create a cache with capacity, TTL and key normalization settings |
| `get(key)` / `put(key, value)` | Lookup (returns `std::optional<std::string>`) and insert |
| `get_or_compute(key, fn)` | Cached value, or the result of `fn()` stored for next time |
| `invalidate(key)`, `clear()`, `size()`, `stats()` | Maintenance and counters |
| `prompt_hash(prompt)` | FNV-1a 64-bit hex digest of a string |

## How it works

Entries live in an `unordered_map` paired with a `std::list` that tracks recency. A hit moves the key to the front; inserting at capacity evicts from the back. Expired entries are removed when they are looked up. All public methods take a mutex.

## Examples

The [`examples/`](examples) folder has runnable programs:

- [`basic_cache.cpp`](examples/basic_cache.cpp)
- [`cached_openai.cpp`](examples/cached_openai.cpp)
- [`lru_eviction.cpp`](examples/lru_eviction.cpp)

Build the examples with CMake:

```bash
cmake -B build
cmake --build build
```

## Limitations

- Exact-match keys (after optional lowercasing). It is not a semantic or embedding-based cache.
- In-memory only; nothing is persisted across restarts.
- `get_or_compute` releases the lock while `fn` runs, so two threads missing the same key at once may both call the API.

## License

MIT. See [LICENSE](LICENSE).
