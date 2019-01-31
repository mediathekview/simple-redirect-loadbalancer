cmake_minimum_required(VERSION 3.10)
project(mv_redirect_server)

set(CMAKE_CXX_STANDARD 17)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -flto -funroll-loops")

find_package(Boost REQUIRED COMPONENTS system coroutine program_options)
find_package(CURL REQUIRED)

add_executable(mv_redirect_server main.cpp lambda.h ServerData.h watchdog_handler.cpp watchdog_handler.h utils.cpp utils.h)

include_directories(${CURL_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
add_definitions(${Boost_DEFINITIONS})
add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

target_link_libraries(mv_redirect_server ${Boost_LIBRARIES} ${CURL_LIBRARIES})
