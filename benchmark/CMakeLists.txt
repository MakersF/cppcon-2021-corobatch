add_library( common STATIC common.cpp )
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library( comparison comparison.cpp )
target_include_directories(comparison PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries( comparison PUBLIC common corobatch )

add_executable( coro_benchmark benchmark.cpp )
target_link_libraries( coro_benchmark PRIVATE comparison benchmark::benchmark )
