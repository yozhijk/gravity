include_directories(inc inc/internal .)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/gtest-all.cc)

add_library(gtest STATIC ${SOURCE_FILES})
