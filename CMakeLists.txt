# cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++-11 -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

cmake_minimum_required(VERSION 3.5)
project ( COROBATCH CXX )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-pedantic -Wall -Wextra -Wuninitialized -std=gnu++2a -fcoroutines-ts -pthread -nostdinc++ -isystem/usr/lib/llvm-11/include/c++/v1/)
add_compile_options( -O3 -g )
SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -L/usr/lib/llvm-11/lib/")

SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -lpthread -lc++ -Wl,-rpath,/usr/lib/llvm-11/lib/")

# add_library( corobatch INTERFACE )
# target_include_directories( corobatch INTERFACE corobatch/include )

# add_subdirectory(include)
# add_subdirectory(src)
# add_subdirectory(benchmark)

add_executable( main src/corobatch.cpp )
# target_link_libraries( main PRIVATE corobatch )
