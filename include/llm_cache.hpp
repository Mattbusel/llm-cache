#pragma once

// llm_cache.hpp -- Zero-dependency single-header C++ semantic cache for LLM responses.
//
// USAGE:
//   In exactly ONE translation unit:
//     #define LLM_CACHE_IMPLEMENTATION
//     #include "llm_cache.hpp"
//
//   In all other translation units:
//     #include "llm_cache.hpp"
//
// Requires: C++17, std::filesystem (links with -lstdc++fs on older GCC)

#include <cstdint>
#include <ctime>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace llm {

// ---------------------------------------------------------------------------
// Public types
// ---------------------------------------------------------------------------

struct CacheConfig {
    std::string cache_dir            = ".llm_cache"; // where to store cache files
    size_t      max_entries          = 1000;          // evict LRU beyond this
    int         ttl_seconds          = 86400;         // 0 = never expire
    double      similarity_threshold = 0.95;          // 0.0 = exact-match only
};

struct CacheEntry {
    std::string prompt;
    std::string response;
    std::time_t created_at = 0;
    size_t      hit_count  = 0;
};

// ---------------------------------------------------------------------------
// Cache class
// ---------------------------------------------------------------------------

class Cache {
public:
    explicit Cache(CacheConfig config = {});

    /// Returns cached response or std::nullopt on miss / expiry.
    std::optional<std::string> get(const std::string& prompt);

    /// Store a prompt -> response mapping.
    void put(const std::string& prompt, const std::string& response);

    /// Remove a specific prompt from the cache.
    void invalidate(const std::string& prompt);

    /// Remove all entries from the cache and disk.
    void clear();

    struct Stats {
        size_t hits          = 0;
        size_t misses        = 0;
        size_t total_entries = 0;
        double hit_rate      = 0.0;
        size_t bytes_saved   = 0; // estimated: hits * avg_response_bytes
    };

    Stats stats() const;

private:
    CacheConfig m_cfg;
    mutable std::mutex m_mu;

    struct Record {
        CacheEntry          entry;
        std::string         hash;
        std::chrono::steady_clock::time_point last_access;
    };

    std::unordered_map<std::string, Record> m_map;   // hash -> record
    std::map<std::chrono::steady_clock::time_point,
             std::string>                   m_lru;   // time -> hash

    mutable size_t m_hits        = 0;
    mutable size_t m_misses      = 0;
    mutable size_t m_bytes_saved = 0;

    std::string hash_prompt(const std::string& prompt) const;
    std::string cache_path(const std::string& hash) const;
    bool load_from_disk(const std::string& hash, CacheEntry& out) const;
    void save_to_disk(const std::string& hash, const CacheEntry& entry) const;
    void delete_from_disk(const std::string& hash) const;
    void evict_lru_locked();
    double jaccard_similarity(const std::string& a, const std::string& b) const;
    std::optional<std::string> find_similar_locked(const std::string& prompt) const;
};

// ---------------------------------------------------------------------------
// Convenience: check cache, call fn on miss, store result
// ---------------------------------------------------------------------------

/// Returns cached response if available, otherwise calls fn(prompt),
/// stores the result, and returns it.
std::string cached_call(
    Cache& cache,
    const std::string& prompt,
    std::function<std::string(const std::string&)> fn
);

} // namespace llm

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

#ifdef LLM_CACHE_IMPLEMENTATION

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace llm {
namespace detail {

// ---------------------------------------------------------------------------
// FNV-1a 64-bit hash
// ---------------------------------------------------------------------------
static uint64_t fnv1a(const std::string& s) {
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : s) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

static std::string hash_to_hex(uint64_t h) {
    char buf[17];
    snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(h));
    return buf;
}

// ---------------------------------------------------------------------------
// Minimal hand-rolled JSON helpers (no external lib)
// ---------------------------------------------------------------------------
static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

static std::string json_unescape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[++i]) {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case '/':  out += '/';  break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;
                default:   out += s[i]; break;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

// Extract the value of a JSON string field: "key": "value"
static std::string json_str(std::string_view json, std::string_view key) {
    std::string needle = "\"";
    needle += key;
    needle += "\"";
    auto pos = json.find(needle);
    if (pos == std::string_view::npos) return {};
    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) ++pos;
    if (pos >= json.size() || json[pos] != '"') return {};
    ++pos;
    std::string val;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            ++pos;
            switch (json[pos]) {
                case '"':  val += '"';  break;
                case '\\': val += '\\'; break;
                case 'n':  val += '\n'; break;
                case 'r':  val += '\r'; break;
                case 't':  val += '\t'; break;
                default:   val += json[pos]; break;
            }
        } else {
            val += json[pos];
        }
        ++pos;
    }
    return val;
}

// Extract a numeric field value as int64
static int64_t json_int(std::string_view json, std::string_view key) {
    std::string needle = "\"";
    needle += key;
    needle += "\"";
    auto pos = json.find(needle);
    if (pos == std::string_view::npos) return 0;
    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return 0;
    int64_t val = 0;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
        val = val * 10 + (json[pos++] - '0');
    }
    return val;
}

// Serialize a CacheEntry to JSON
static std::string entry_to_json(const CacheEntry& e) {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"prompt\": \""    << json_escape(e.prompt)   << "\",\n"
       << "  \"response\": \""  << json_escape(e.response) << "\",\n"
       << "  \"created_at\": "  << static_cast<int64_t>(e.created_at) << ",\n"
       << "  \"hit_count\": "   << e.hit_count << "\n"
       << "}\n";
    return ss.str();
}

// Deserialize a CacheEntry from JSON
static bool entry_from_json(const std::string& json, CacheEntry& out) {
    out.prompt     = json_str(json, "prompt");
    out.response   = json_str(json, "response");
    out.created_at = static_cast<std::time_t>(json_int(json, "created_at"));
    out.hit_count  = static_cast<size_t>(json_int(json, "hit_count"));
    return !out.prompt.empty() && !out.response.empty();
}

// ---------------------------------------------------------------------------
// Shingling: character 3-gram Jaccard similarity
// ---------------------------------------------------------------------------
static std::set<std::string> shingles(const std::string& s, int k = 3) {
    std::set<std::string> out;
    if (static_cast<int>(s.size()) < k) { out.insert(s); return out; }
    for (size_t i = 0; i + static_cast<size_t>(k) <= s.size(); ++i)
        out.insert(s.substr(i, static_cast<size_t>(k)));
    return out;
}

static double jaccard(const std::string& a, const std::string& b) {
    auto sa = shingles(a);
    auto sb = shingles(b);
    size_t intersect = 0;
    for (const auto& s : sa)
        if (sb.count(s)) ++intersect;
    size_t uni = sa.size() + sb.size() - intersect;
    return uni == 0 ? 1.0 : static_cast<double>(intersect) / static_cast<double>(uni);
}

} // namespace detail

// ---------------------------------------------------------------------------
// Cache implementation
// ---------------------------------------------------------------------------

Cache::Cache(CacheConfig config) : m_cfg(std::move(config)) {
    std::filesystem::create_directories(m_cfg.cache_dir);
    // Load existing cache files from disk into memory index
    for (const auto& entry : std::filesystem::directory_iterator(m_cfg.cache_dir)) {
        if (entry.path().extension() != ".json") continue;
        std::string hash = entry.path().stem().string();
        CacheEntry ce;
        if (load_from_disk(hash, ce)) {
            Record rec;
            rec.entry       = std::move(ce);
            rec.hash        = hash;
            rec.last_access = std::chrono::steady_clock::now();
            m_lru[rec.last_access] = hash;
            m_map[hash] = std::move(rec);
        }
    }
}

std::string Cache::hash_prompt(const std::string& prompt) const {
    return detail::hash_to_hex(detail::fnv1a(prompt));
}

std::string Cache::cache_path(const std::string& hash) const {
    return m_cfg.cache_dir + "/" + hash + ".json";
}

bool Cache::load_from_disk(const std::string& hash, CacheEntry& out) const {
    std::ifstream f(cache_path(hash));
    if (!f.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return detail::entry_from_json(content, out);
}

void Cache::save_to_disk(const std::string& hash, const CacheEntry& entry) const {
    std::ofstream f(cache_path(hash));
    if (f.is_open()) f << detail::entry_to_json(entry);
}

void Cache::delete_from_disk(const std::string& hash) const {
    std::filesystem::remove(cache_path(hash));
}

void Cache::evict_lru_locked() {
    while (m_map.size() > m_cfg.max_entries && !m_lru.empty()) {
        auto it = m_lru.begin();
        std::string evict_hash = it->second;
        m_lru.erase(it);
        delete_from_disk(evict_hash);
        m_map.erase(evict_hash);
    }
}

double Cache::jaccard_similarity(const std::string& a, const std::string& b) const {
    return detail::jaccard(a, b);
}

std::optional<std::string> Cache::find_similar_locked(const std::string& prompt) const {
    if (m_cfg.similarity_threshold <= 0.0) return std::nullopt;
    std::string best_hash;
    double best_sim = m_cfg.similarity_threshold;
    for (const auto& [hash, rec] : m_map) {
        double sim = jaccard_similarity(prompt, rec.entry.prompt);
        if (sim >= best_sim) {
            best_sim = sim;
            best_hash = hash;
        }
    }
    if (best_hash.empty()) return std::nullopt;
    return m_map.at(best_hash).entry.response;
}

std::optional<std::string> Cache::get(const std::string& prompt) {
    std::lock_guard<std::mutex> lock(m_mu);
    std::string hash = hash_prompt(prompt);

    auto it = m_map.find(hash);
    if (it != m_map.end()) {
        // Check TTL
        if (m_cfg.ttl_seconds > 0) {
            std::time_t now = std::time(nullptr);
            if (now - it->second.entry.created_at > m_cfg.ttl_seconds) {
                delete_from_disk(hash);
                m_lru.erase(it->second.last_access);
                m_map.erase(it);
                ++m_misses;
                return std::nullopt;
            }
        }
        // Update LRU
        m_lru.erase(it->second.last_access);
        it->second.last_access = std::chrono::steady_clock::now();
        m_lru[it->second.last_access] = hash;
        ++it->second.entry.hit_count;
        m_bytes_saved += it->second.entry.response.size();
        ++m_hits;
        return it->second.entry.response;
    }

    // Fuzzy fallback
    if (m_cfg.similarity_threshold > 0.0) {
        auto fuzzy = find_similar_locked(prompt);
        if (fuzzy) {
            ++m_hits;
            m_bytes_saved += fuzzy->size();
            return fuzzy;
        }
    }

    ++m_misses;
    return std::nullopt;
}

void Cache::put(const std::string& prompt, const std::string& response) {
    std::lock_guard<std::mutex> lock(m_mu);
    std::string hash = hash_prompt(prompt);

    CacheEntry entry;
    entry.prompt     = prompt;
    entry.response   = response;
    entry.created_at = std::time(nullptr);
    entry.hit_count  = 0;

    Record rec;
    rec.entry       = entry;
    rec.hash        = hash;
    rec.last_access = std::chrono::steady_clock::now();

    // Remove old LRU entry if re-inserting
    if (m_map.count(hash)) {
        m_lru.erase(m_map.at(hash).last_access);
    }

    m_map[hash]          = rec;
    m_lru[rec.last_access] = hash;

    save_to_disk(hash, entry);
    evict_lru_locked();
}

void Cache::invalidate(const std::string& prompt) {
    std::lock_guard<std::mutex> lock(m_mu);
    std::string hash = hash_prompt(prompt);
    auto it = m_map.find(hash);
    if (it != m_map.end()) {
        m_lru.erase(it->second.last_access);
        delete_from_disk(hash);
        m_map.erase(it);
    }
}

void Cache::clear() {
    std::lock_guard<std::mutex> lock(m_mu);
    for (const auto& [hash, rec] : m_map) delete_from_disk(hash);
    m_map.clear();
    m_lru.clear();
}

Cache::Stats Cache::stats() const {
    std::lock_guard<std::mutex> lock(m_mu);
    Stats s;
    s.hits          = m_hits;
    s.misses        = m_misses;
    s.total_entries = m_map.size();
    size_t total    = m_hits + m_misses;
    s.hit_rate      = total > 0 ? static_cast<double>(m_hits) / static_cast<double>(total) : 0.0;
    s.bytes_saved   = m_bytes_saved;
    return s;
}

std::string cached_call(Cache& cache, const std::string& prompt,
                        std::function<std::string(const std::string&)> fn) {
    auto cached = cache.get(prompt);
    if (cached) return *cached;
    std::string result = fn(prompt);
    if (!result.empty()) cache.put(prompt, result);
    return result;
}

} // namespace llm

#endif // LLM_CACHE_IMPLEMENTATION
