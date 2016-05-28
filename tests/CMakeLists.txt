include_directories(../gtest/inc ../gravity/inc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES tests_main.cpp)

add_executable(tests ${SOURCE_FILES})

target_link_libraries(tests gravity gtest)

