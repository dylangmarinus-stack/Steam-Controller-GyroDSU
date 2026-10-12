cmake_minimum_required(VERSION 3.16)
project(sc2gyrodsu CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

find_library(HIDAPI_LIB hidapi-hidraw REQUIRED)
find_path(HIDAPI_INC hidapi/hidapi.h REQUIRED)

add_executable(sc2gyrodsu
    src/main.cpp
    src/hiddev.cpp
    src/triton.cpp
    src/dsu.cpp
)

target_include_directories(sc2gyrodsu PRIVATE inc ${HIDAPI_INC})
target_link_libraries(sc2gyrodsu PRIVATE
    Threads::Threads
    ${HIDAPI_LIB}
)
target_compile_options(sc2gyrodsu PRIVATE -O3 -Wall)

include(CTest)
if(BUILD_TESTING)
    add_executable(hiddev_test tests/hiddev_test.cpp src/hiddev.cpp src/triton.cpp)
    target_include_directories(hiddev_test PRIVATE inc ${HIDAPI_INC})
    target_compile_options(hiddev_test PRIVATE -Wall)
    add_test(NAME hiddev COMMAND hiddev_test)
endif()
