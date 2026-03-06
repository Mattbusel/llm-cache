# AGENTS.md — llm-cache

## Purpose

Single-header C++ LLM response cache. Saves API calls by returning stored results
for prompts already seen. Supports exact-match and fuzzy (shingling) matching,
LRU eviction, TTL expiry, and file-backed persistence.

## Architecture

```
llm-cache/
  include/
    llm_cache.hpp       <- THE ENTIRE LIBRARY. Do not split.
  examples/
    basic_cache.cpp     <- Demo: put/get, stats, cached_call
    cached_openai.cpp   <- Demo: wraps llm-stream OpenAI calls with cache
  CMakeLists.txt
  README.md / AGENTS.md / CLAUDE.md / LICENSE
```

## Build

```bash
cmake -B build && cmake --build build
```

## Rules

- Single header. Never split llm_cache.hpp.
- No external deps (libcurl only in cached_openai example).
- All public API in namespace `llm`.
- Thread-safe: std::mutex guards all cache operations.
- File I/O via std::fstream only.
- C++17, zero warnings with -Wall -Wextra.
