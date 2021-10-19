# cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++-11 -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBENCHMARK_ENABLE_GTEST_TESTS=OFF ..
# Make sure to clone google-benchmark inside the vendored/benchmark folder

cmake_minimum_required(VERSION 3.9)

project ( COROBATCH CXX )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-pedantic -Wall -Wextra -Wuninitialized -std=gnu++2a -pthread)
#add_compile_options(-pedantic -Wall -Wextra -Wuninitialized -std=gnu++2a -fcoroutines-ts -pthread -nostdinc++ -isystem/usr/lib/llvm-11/include/c++/v1/)
# SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -L/usr/lib/llvm-11/lib/")

add_subdirectory(vendored/benchmark)
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# add_compile_options( -fsanitize=address )
# SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -fsanitize=address")

# SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -lpthread -lc++ -Wl,-rpath,/usr/lib/llvm-11/lib/")

add_library(corobatch STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/corobatch_opt.cpp)
target_include_directories(corobatch INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(benchmark)

add_executable( test src/test.cpp )
target_link_libraries( test PRIVATE corobatch )

add_executable( test_opt src/test_opt.cpp )
target_link_libraries( test_opt PRIVATE corobatch )
