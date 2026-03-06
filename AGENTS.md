# AGENTS.md — llm-cache

## Purpose
Single-header C++ LRU cache for LLM responses with TTL expiry and thread safety.

## Architecture
Everything in `include/llm_cache.hpp`. Implementation guard: `#ifdef LLM_CACHE_IMPLEMENTATION`.

## Build
```bash
cmake -B build && cmake --build build
```

## Constraints
- Single header, no external deps, C++17
- Thread-safe via std::mutex
- namespace llm
