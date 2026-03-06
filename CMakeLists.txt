cmake_minimum_required(VERSION 3.16)
project(llm_cache_examples CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

add_executable(basic_cache examples/basic_cache.cpp)
target_include_directories(basic_cache PRIVATE include)

add_executable(lru_eviction examples/lru_eviction.cpp)
target_include_directories(lru_eviction PRIVATE include)
