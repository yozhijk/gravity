include_directories(inc inc/internal .)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/gtest-all.cc)

find_package( Threads )

add_library(gtest STATIC ${SOURCE_FILES})

target_link_libraries( gtest ${CMAKE_THREAD_LIBS_INIT} )

