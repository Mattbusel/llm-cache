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

| Repo | Purpose |
|------|---------|
| [llm-stream](https://github.com/Mattbusel/llm-stream) | SSE streaming |
| [llm-cache](https://github.com/Mattbusel/llm-cache) | Response caching |
| [llm-cost](https://github.com/Mattbusel/llm-cost) | Token cost estimation |
| [llm-retry](https://github.com/Mattbusel/llm-retry) | Retry + circuit breaker |
| [llm-format](https://github.com/Mattbusel/llm-format) | Markdown/code formatting |
| [llm-embed](https://github.com/Mattbusel/llm-embed) | Embeddings + cosine similarity |
| [llm-pool](https://github.com/Mattbusel/llm-pool) | Connection pooling |
| [llm-log](https://github.com/Mattbusel/llm-log) | Structured logging |
| [llm-template](https://github.com/Mattbusel/llm-template) | Prompt templates |
| [llm-agent](https://github.com/Mattbusel/llm-agent) | Tool-use agent loop |
| [llm-rag](https://github.com/Mattbusel/llm-rag) | Retrieval-augmented generation |
| [llm-eval](https://github.com/Mattbusel/llm-eval) | Output evaluation |
| [llm-chat](https://github.com/Mattbusel/llm-chat) | Multi-turn chat |
| [llm-vision](https://github.com/Mattbusel/llm-vision) | Vision/image inputs |
| [llm-mock](https://github.com/Mattbusel/llm-mock) | Mock LLM for testing |
| [llm-router](https://github.com/Mattbusel/llm-router) | Model routing |
| [llm-guard](https://github.com/Mattbusel/llm-guard) | Content moderation |
| [llm-compress](https://github.com/Mattbusel/llm-compress) | Prompt compression |
| [llm-batch](https://github.com/Mattbusel/llm-batch) | Batch processing |
| [llm-audio](https://github.com/Mattbusel/llm-audio) | Audio transcription/TTS |
| [llm-finetune](https://github.com/Mattbusel/llm-finetune) | Fine-tuning jobs |
| [llm-rank](https://github.com/Mattbusel/llm-rank) | Passage reranking |
| [llm-parse](https://github.com/Mattbusel/llm-parse) | HTML/markdown parsing |
| [llm-trace](https://github.com/Mattbusel/llm-trace) | Distributed tracing |
| [llm-ab](https://github.com/Mattbusel/llm-ab) | A/B testing |
| [llm-json](https://github.com/Mattbusel/llm-json) | JSON parsing/building |

## License

MIT — see [LICENSE](LICENSE).
