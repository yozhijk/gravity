include_directories(inc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/sg.cpp)

add_library(gravity STATIC ${SOURCE_FILES})
