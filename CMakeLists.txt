cmake_minimum_required(VERSION 3.16)
project(llm_cache_examples CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(basic_cache examples/basic_cache.cpp)
target_include_directories(basic_cache PRIVATE include)

find_package(CURL QUIET)
if(CURL_FOUND)
    add_executable(cached_openai examples/cached_openai.cpp)
    target_include_directories(cached_openai PRIVATE include)
    target_link_libraries(cached_openai CURL::libcurl)
endif()
