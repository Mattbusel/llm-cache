# CLAUDE.md — llm-cache

## Build & Run
```bash
cmake -B build && cmake --build build
./build/basic_cache
./build/lru_eviction
```

## THE RULE: Single Header
`include/llm_cache.hpp` is the entire library.

## API to maintain
- `ResponseCache(CacheConfig)` — construct with config
- `get(key)` → `std::optional<std::string>`
- `put(key, value)` → void
- `invalidate(key)` → bool
- `clear()` → void
- `stats()` → `CacheStats`
- `get_or_compute(key, fn)` → `std::string`
- `prompt_hash(prompt)` → `std::string` (FNV-1a hex)

## Common mistakes
- Forgetting `#define LLM_CACHE_IMPLEMENTATION` in one TU
- Calling `get()` and `put()` in the same lock scope (deadlock) — each acquires its own lock
