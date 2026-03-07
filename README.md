# llm-cache

LRU cache for LLM responses. Single header, no deps, thread-safe.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![License MIT](https://img.shields.io/badge/license-MIT-green.svg)
![Single Header](https://img.shields.io/badge/single-header-orange.svg)

## Quickstart

```cpp
#define LLM_CACHE_IMPLEMENTATION
#include "llm_cache.hpp"

llm::ResponseCache cache;

std::string response = cache.get_or_compute("What is 2+2?", [&]() {
    return call_my_llm("What is 2+2?");  // only called on miss
});
```

## Installation

Copy `include/llm_cache.hpp`. No external dependencies.

## API

```cpp
llm::CacheConfig cfg;
cfg.max_entries  = 1000;     // LRU eviction above this
cfg.ttl_seconds  = 3600.0;  // 0 = never expire
cfg.case_sensitive = false;  // normalize keys to lowercase

llm::ResponseCache cache(cfg);

cache.put("prompt", "response");
auto val = cache.get("prompt");   // std::optional<std::string>
cache.invalidate("prompt");
cache.clear();

// Cache stats
auto s = cache.stats();
s.hits; s.misses; s.evictions; s.hit_rate();

// One-liner: get cached or call fn on miss
std::string r = cache.get_or_compute("prompt", []{ return call_llm(); });

// Hash a prompt for use as a compact key
std::string key = llm::prompt_hash("long prompt text...");
```

## Examples

- [`examples/basic_cache.cpp`](examples/basic_cache.cpp) — get_or_compute, hit rate stats
- [`examples/lru_eviction.cpp`](examples/lru_eviction.cpp) — demonstrate LRU eviction with tiny cache

## Building

```bash
cmake -B build && cmake --build build
./build/basic_cache
./build/lru_eviction
```

## Requirements

C++17. No external dependencies.

## See Also

| Repo | What it does |
|------|-------------|
| [llm-stream](https://github.com/Mattbusel/llm-stream) | Stream OpenAI and Anthropic responses via SSE |
| [llm-cache](https://github.com/Mattbusel/llm-cache) | LRU response cache |
| [llm-cost](https://github.com/Mattbusel/llm-cost) | Token counting and cost estimation |
| [llm-retry](https://github.com/Mattbusel/llm-retry) | Retry and circuit breaker |
| [llm-format](https://github.com/Mattbusel/llm-format) | Structured output / JSON schema |
| [llm-embed](https://github.com/Mattbusel/llm-embed) | Embeddings and vector search |
| [llm-pool](https://github.com/Mattbusel/llm-pool) | Concurrent request pool |
| [llm-log](https://github.com/Mattbusel/llm-log) | Structured JSONL logging |
| [llm-template](https://github.com/Mattbusel/llm-template) | Prompt templating |
| [llm-agent](https://github.com/Mattbusel/llm-agent) | Tool-calling agent loop |
| [llm-rag](https://github.com/Mattbusel/llm-rag) | RAG pipeline |
| [llm-eval](https://github.com/Mattbusel/llm-eval) | Evaluation and consistency scoring |
| [llm-chat](https://github.com/Mattbusel/llm-chat) | Conversation memory manager |
| [llm-vision](https://github.com/Mattbusel/llm-vision) | Multimodal image+text |
| [llm-mock](https://github.com/Mattbusel/llm-mock) | Mock LLM for testing |
| [llm-router](https://github.com/Mattbusel/llm-router) | Model routing by complexity |
| [llm-guard](https://github.com/Mattbusel/llm-guard) | PII detection and injection guard |
| [llm-compress](https://github.com/Mattbusel/llm-compress) | Context compression |
| [llm-batch](https://github.com/Mattbusel/llm-batch) | Batch processing and checkpointing |

## License

MIT — see [LICENSE](LICENSE).
